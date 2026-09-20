#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent

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

    print(f"==> Configuring preset '{args.preset}'", flush=True)
    run(["cmake", "--preset", args.preset])

    print(f"==> Building preset '{args.preset}' with {args.jobs} job(s)", flush=True)
    run(["cmake", "--build", "--preset", args.preset, "-j", str(args.jobs)])

    refresh_compile_commands_symlink(args.preset)

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


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
