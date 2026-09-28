#!/usr/bin/env python3

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
SOURCE_DIRS: tuple[str, ...] = ("include", "src", "tests", "sandbox")
SOURCE_SUFFIXES: tuple[str, ...] = (".h", ".hpp", ".cpp")
TIDY_DIAGNOSTIC = re.compile(
    r"^(?P<path>.+?):(?P<line>\d+):(?P<column>\d+): (?P<level>warning|error): .* \[(?P<check>[^\]]+)\]$"
)

# Tests creating threads or a parking lot should be ran isolated to not mess
# with the global counter on thread index and introduce a subtle error
ISOLATED_TESTS: tuple[str, ...] = (
    "SpinLockMultiThreadTest.LockIncrementsNeverRace",
    "SpinLockMultiThreadTest.TryLockIncrementsNeverRace",
    "MutexMultiThreadTest.LockIncrementsNeverRace",
    "LatchMultiThreadTest.CountDownsNeverRace",
    "LatchMultiThreadTest.WaitUnblocksAfterCountDown",
    "SemaphoreMultiThreadTest.AcquireReleaseNeverRace",
    "SemaphoreMultiThreadTest.AcquireUnblocksAfterRelease",
    "ConditionVariableSingleThreadTest.NotifyWithNoWaitersIsSafe",
    "ConditionVariableMultiThreadTest.WaitUnblocksOnNotifyOne",
    "ConditionVariableMultiThreadTest.NotifyAllWakesEveryWaiter",
    "EventSingleThreadTest.WaitForTimesOutThenSucceedsAfterTrigger",
    "EventMultiThreadTest.TriggerUnblocksWaiter",
    "EventMultiThreadTest.TriggerUnblocksAllWaiters",
    "ThreadTest.CreateJoinRunsEntry",
    "ThreadTest.SuspendedWaitsForResume",
    "ThreadTest.IndexVisibleInEntry",
    "ThreadTest.IndicesAreDistinct",
    "ThreadTest.CurrentResolvesToSelf",
    "ThreadTest.AdoptCapturesCallingThread",
    "TaskSystemTest.LaunchWithoutPrerequisites",
    "TaskSystemTest.PrerequisiteRunsBeforeDependant",
    "TaskSystemTest.MultiplePrerequisitesRunBeforeDependant",
    "TaskSystemTest.WaitOnSpanWaitsForAll",
    "TaskSystemTest.ReleaseSucceedsAfterCompletion",
    "TaskSystemTest.ReleaseFailsBeforeCompletion",
    "TaskSystemTest.LaunchDeducesVoidTaskType",
    "TaskSystemTest.LaunchDeducesValueTaskType",
    "TaskSystemTest.GetResultWaitsAndReturnsInline",
    "TaskSystemTest.GetResultReturnsHeapStoredResult",
    "TaskSystemTest.GetResultReturnsVector",
    "TaskSystemTest.TypedTaskCanBeUsedAsPrerequisite",
    "TaskSystemTest.TypedTaskExposesUnderlyingHandle",
    "TaskSystemTest.TypedResultReleasedAfterAccess",
    "TaskSystemTest.LaunchFailsWhenCapacityExhausted",
    "TaskSystemMultiThreadTest.IndependentTasksAllCompleteOnce",
    "TaskSystemMultiThreadTest.TasksDistributeAcrossWorkers",
    "TaskSystemMultiThreadTest.DependencyChainPreservesOrder",
    "TaskSystemMultiThreadTest.FanOutCompletesBeforeFanIn",
    "TaskSystemMultiThreadTest.ReentrantWaitDoesNotDeadlock",
    "TaskSystemMultiThreadTest.DestructorDrainsOutstandingWork",
    "TaskSystemMultiThreadTest.DestructorWaitsForSlowTask",
    "SegmentAllocatorMultiThreadTest.AllocFreeNeverHandsOutSameSegment",
    "SegmentAllocatorMultiThreadTest.MultiSegmentRunsStayContiguous",
    "SegmentAllocatorMultiThreadTest.CommitKeepsPrefixConsistent",
    "SegmentAllocatorMultiThreadTest.CommittedBytesReturnsToBaseline",
)

# Deliberately slow (wall-clock waits, not just heavier work) - skipped by
# default. NOT FOR STRESS TESTS THEY SHOULD NOT BE SKIPPED
LONG_TESTS: tuple[str, ...] = ()


def default_jobs() -> int:
    return os.cpu_count() or 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-p", "--preset", default="debug", help="CMake preset to configure/build (default: %(default)s)"
    )
    parser.add_argument(
        "-j", "--jobs", type=int, default=default_jobs(), help="Parallel compile jobs (default: all logical cores)"
    )
    parser.add_argument("-t", "--test", action="store_true", help="Run the test suite after building")
    parser.add_argument(
        "-l", "--long-tests", action="store_true", help="Also run long-running tests (skipped by default)"
    )
    parser.add_argument("-r", "--run-sandbox", action="store_true", help="Run the sandbox after building")
    parser.add_argument(
        "-n", "--repeat", type=int, default=1, help="Run the test suite this many times (for stress-testing flakiness)"
    )
    parser.add_argument("-f", "--format", action="store_true", help="Apply clang-format to all sources before building")
    parser.add_argument(
        "--format-check", action="store_true", help="Report sources that are not clang-formatted, without changing them"
    )
    parser.add_argument(
        "--tidy", action="store_true", help="Run clang-tidy over all sources after building (list in build/<preset>)"
    )
    # help done command automatically

    return parser.parse_args()


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT_DIR, check=True)  # noqa: S603 - fixed argv, no shell, no untrusted input


def refresh_compile_commands_symlink(preset: str) -> None:
    target = Path("build") / preset / "compile_commands.json"
    link = ROOT_DIR / "compile_commands.json"

    if not (ROOT_DIR / target).is_file():
        print(f"==> Warning: {target} not found, leaving symlink untouched", file=sys.stderr)
        return

    print(f"==> Refreshing compile_commands.json symlink -> {target}", flush=True)
    if link.is_symlink() or link.exists():
        link.unlink()

    try:
        link.symlink_to(target)
    except OSError:
        # Most likely Windows without symlink privilege (Developer Mode/elevation)
        print("==> Warning: could not create symlink, copying file instead", file=sys.stderr)
        shutil.copyfile(ROOT_DIR / target, link)


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        print(f"==> Error: {name} not found on PATH", file=sys.stderr)
        sys.exit(1)
    return path


def project_sources() -> list[Path]:
    return sorted(
        path
        for directory in SOURCE_DIRS
        for path in (ROOT_DIR / directory).rglob("*")
        if path.suffix in SOURCE_SUFFIXES and path.is_file() and "build" not in path.relative_to(ROOT_DIR).parts
    )


def run_format(check_only: bool) -> bool:
    clang_format = require_tool("clang-format")
    sources = [str(path) for path in project_sources()]

    if not check_only:
        print(f"==> Formatting {len(sources)} file(s)", flush=True)
        run([clang_format, "-i", *sources])
        return True

    print(f"==> Checking formatting of {len(sources)} file(s)", flush=True)
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [clang_format, "--dry-run", "-Werror", *sources],
        cwd=ROOT_DIR,
        check=False,
        capture_output=True,
        text=True,
    )
    unformatted = sorted({line.split(":", 1)[0] for line in result.stderr.splitlines() if ": error: " in line})
    for path in unformatted:
        print(f"    {Path(path).relative_to(ROOT_DIR)}")
    print(f"    [{'PASS' if not unformatted else 'FAIL'}] {len(unformatted)} file(s) need formatting")
    return not unformatted


def tidy_extra_args() -> list[str]:
    if sys.platform != "darwin":
        return []
    xcrun = require_tool("xcrun")
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [xcrun, "--show-sdk-path"],
        check=True,
        capture_output=True,
        text=True,
    )
    return [f"--extra-arg=-isysroot{result.stdout.strip()}"]


def is_project_path(path: Path) -> bool:
    return any(path.is_relative_to(ROOT_DIR / directory) for directory in SOURCE_DIRS)


def run_tidy(build_dir: Path, jobs: int) -> bool:
    clang_tidy = require_tool("clang-tidy")
    commands = json.loads((build_dir / "compile_commands.json").read_text())
    sources = sorted({path for entry in commands if is_project_path(path := Path(entry["file"]).resolve())})
    extra_args = tidy_extra_args()

    def tidy_file(source: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
            [clang_tidy, "-p", str(build_dir), "--quiet", *extra_args, str(source)],
            cwd=ROOT_DIR,
            check=False,
            capture_output=True,
            text=True,
        )

    print(f"==> Running clang-tidy over {len(sources)} translation unit(s) with {jobs} job(s)", flush=True)
    with ThreadPoolExecutor(jobs) as pool:
        results = list(pool.map(tidy_file, sources))

    config_errors = sorted(
        {line for result in results for line in result.stderr.splitlines() if "clang-tidy-config" in line}
    )
    for line in config_errors:
        print(f"    {line}", file=sys.stderr)

    diagnostics: dict[tuple[str, int, int, str], str] = {}
    for result in results:
        for line in result.stdout.splitlines():
            match = TIDY_DIAGNOSTIC.match(line)
            if match is None or not is_project_path(Path(match["path"]).resolve()):
                continue
            key = (match["path"], int(match["line"]), int(match["column"]), match["check"])
            diagnostics[key] = line

    report = build_dir / "clang-tidy.txt"
    report.write_text("".join(f"{diagnostics[key]}\n" for key in sorted(diagnostics)))

    counts = Counter(check for _, _, _, check in diagnostics)
    for check, count in counts.most_common():
        print(f"    {count:5}  {check}")

    errors = sum(1 for line in diagnostics.values() if ": error: " in line)
    passed = errors == 0 and not config_errors
    print(f"    [{'PASS' if passed else 'FAIL'}] {len(diagnostics)} diagnostic(s), {errors} error(s)")
    print(f"    Full list: {report.relative_to(ROOT_DIR)}")
    return passed


def test_binary_path(build_dir: Path) -> Path:
    executable = "trivial_tests.exe" if os.name == "nt" else "trivial_tests"
    return build_dir / "tests" / executable


def sandbox_binary_path(build_dir: Path) -> Path:
    executable = "trivial_sandbox.exe" if os.name == "nt" else "trivial_sandbox"
    return build_dir / "sandbox" / executable


def extract_count(output: str, marker: str, index: int) -> int:
    for line in output.splitlines():
        if marker in line:
            return int(line.split()[index])
    msg = f"could not find a line containing {marker!r} in gtest output - see if its summary format change"
    raise RuntimeError(msg)


def run_gtest(binary: str, gtest_filter: str) -> tuple[int, str]:
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [binary, f"--gtest_filter={gtest_filter}", "--gtest_brief=1"],
        cwd=ROOT_DIR,
        check=False,
        capture_output=True,
        text=True,
    )
    return result.returncode, result.stdout + result.stderr


def run_tests(build_dir: Path, include_long: bool, indent: str = "") -> bool:
    binary = str(test_binary_path(build_dir))

    if include_long:
        excluded = ISOLATED_TESTS
        isolated = ISOLATED_TESTS
    else:
        if LONG_TESTS:
            print(f"{indent}==> Skipping {len(LONG_TESTS)} long test(s) (pass --long-tests to include)", flush=True)
        excluded = ISOLATED_TESTS + LONG_TESTS
        isolated = tuple(test for test in ISOLATED_TESTS if test not in LONG_TESTS)

    print(f"{indent}==> Running shared-process tests", flush=True)
    exclude_filter = "-" + ":".join(excluded)
    shared_code, shared_output = run_gtest(binary, exclude_filter)
    shared_total = extract_count(shared_output, " ran. (", 1)
    shared_passed = extract_count(shared_output, "[  PASSED  ]", 3)
    if shared_code != 0:
        print(shared_output)
    print(f"{indent}    [{'PASS' if shared_code == 0 else 'FAIL'}] {shared_passed}/{shared_total} tests")

    print(f"{indent}==> Running isolated tests", flush=True)
    isolated_failed = 0
    for test in isolated:
        code, output = run_gtest(binary, test)
        if code != 0:
            isolated_failed += 1
            print(output)
            print(f"{indent}    [FAIL] {test}")
    isolated_total = len(isolated)
    isolated_passed = isolated_total - isolated_failed
    print(f"{indent}    [{'PASS' if isolated_failed == 0 else 'FAIL'}] {isolated_passed}/{isolated_total} tests")

    total = shared_total + isolated_total
    passed = shared_passed + isolated_passed

    print(f"{indent}==> Test summary")
    print(f"{indent}    Shared-process: {shared_passed}/{shared_total} passed")
    print(f"{indent}    Isolated:       {isolated_passed}/{isolated_total} passed")
    print(f"{indent}    Total:          {passed}/{total} passed")

    return shared_code == 0 and isolated_failed == 0


def main() -> None:
    args = parse_args()
    checks_failed = False

    if (args.format or args.format_check) and not run_format(check_only=not args.format):
        checks_failed = True

    print(f"==> Configuring preset '{args.preset}'", flush=True)
    run(["cmake", "--preset", args.preset])

    print(f"==> Building preset '{args.preset}' with {args.jobs} job(s)", flush=True)
    run(["cmake", "--build", "--preset", args.preset, "-j", str(args.jobs)])

    refresh_compile_commands_symlink(args.preset)

    if args.tidy and not run_tidy(ROOT_DIR / "build" / args.preset, args.jobs):
        checks_failed = True

    if args.test:
        failed_runs = 0

        for run_index in range(args.repeat):
            if args.repeat > 1:
                print()
                print(f"==> Test run {run_index + 1}/{args.repeat}", flush=True)

            indent = "    " if args.repeat > 1 else ""
            if not run_tests(ROOT_DIR / "build" / args.preset, args.long_tests, indent):
                failed_runs += 1

        if args.repeat > 1:
            passed_runs = args.repeat - failed_runs
            print()
            print(f"==> Overall: {passed_runs}/{args.repeat} runs passed")

        if failed_runs > 0:
            sys.exit(1)

    if args.run_sandbox:
        print("==> Running sandbox", flush=True)
        run([str(sandbox_binary_path(ROOT_DIR / "build" / args.preset))])

    if checks_failed:
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
