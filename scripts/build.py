#!/usr/bin/env python3

import argparse
import ctypes
import json
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import TextIO

ROOT_DIR = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT_DIR / "build"
SOURCE_DIRS: tuple[str, ...] = ("include", "src", "tests", "sandbox")
HEADER_SUFFIXES: tuple[str, ...] = (".h",)
SOURCE_SUFFIXES: tuple[str, ...] = (".cpp",)

TOOL_VERSIONS: dict[str, str] = {
    "clang-format": "23.1.2",
    "clang-tidy": "23.1.2",
}

ALL_EXCLUDED_PRESETS: tuple[str, ...] = ("debug-deps",)

SANDBOX_SMOKE_SECONDS = 5.0

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
    "EventSingleThreadTest.WaitForMaximumTimeoutReturnsWhenTriggered",
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


class Style(Enum):
    BOLD = "1"
    DIM = "2"
    RED = "31"
    GREEN = "32"
    YELLOW = "33"


class Status(Enum):
    PASSED = "PASS"
    FAILED = "FAIL"
    NOT_RUN = "-"

    @classmethod
    def of(cls, passed: bool) -> Status:
        return cls.PASSED if passed else cls.FAILED

    @property
    def style(self) -> Style:
        return {Status.PASSED: Style.GREEN, Status.FAILED: Style.RED, Status.NOT_RUN: Style.DIM}[self]


@dataclass
class TestRun:
    exit_code: int
    output: str
    reported: bool
    ran: int = 0
    skipped: int = 0
    failures: dict[str, list[str]] = field(default_factory=dict[str, list[str]])

    @property
    def passed(self) -> bool:
        return self.reported and self.exit_code == 0 and not self.failures


@dataclass(frozen=True, order=True)
class DiagnosticKey:
    path: str
    line: int
    column: int
    level: str
    message: str


# -----------------------------------------------------------------------------
# Process and output helpers
# -----------------------------------------------------------------------------


def enable_windows_colour() -> None:
    if sys.platform != "win32":
        return
    # The legacy console host only interprets ANSI codes once asked to
    enable_virtual_terminal_processing = 0x0004
    kernel32 = ctypes.windll.kernel32
    for standard_handle in (-11, -12):
        handle = kernel32.GetStdHandle(standard_handle)
        mode = ctypes.c_uint32()
        if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            kernel32.SetConsoleMode(handle, mode.value | enable_virtual_terminal_processing)


def colour_enabled(stream: TextIO) -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    if os.environ.get("FORCE_COLOR"):
        return True
    return stream.isatty()


def paint(text: str, style: Style, stream: TextIO = sys.stdout) -> str:
    return f"\033[{style.value}m{text}\033[0m" if colour_enabled(stream) else text


def print_step(message: str, indent: str = "") -> None:
    print(indent + paint(f"==> {message}", Style.BOLD))


def print_notice(label: str, message: str, style: Style) -> None:
    print(f"{paint(f'==> {label}:', style, sys.stderr)} {message}", file=sys.stderr)


def print_result(passed: bool, message: str, indent: str = "    ") -> None:
    status = Status.of(passed)
    print(f"{indent}[{paint(status.value, status.style)}] {message}")


def print_table(headers: list[str], rows: list[list[str | Status]], indent: str = "    ") -> None:
    def text(cell: str | Status) -> str:
        return cell.value if isinstance(cell, Status) else cell

    def padded(cell: str | Status, width: int) -> str:
        # Pad before painting, as the escape codes would otherwise count towards the width
        aligned = f"{text(cell):<{width}}"
        return paint(aligned, cell.style) if isinstance(cell, Status) else aligned

    widths = [max(len(text(cell)) for cell in column) for column in zip(headers, *rows, strict=True)]
    border = indent + "+" + "+".join("-" * (width + 2) for width in widths) + "+"

    def print_row(cells: list[str | Status]) -> None:
        print(
            indent + "|" + "|".join(f" {padded(cell, width)} " for cell, width in zip(cells, widths, strict=True)) + "|"
        )

    print(border)
    print_row([*headers])
    print(border)
    for row in rows:
        print_row(row)
    print(border)


def require_tool(name: str) -> str | None:
    path = shutil.which(name)
    if path is None:
        print_notice("Error", f"{name} not found on PATH", Style.RED)
        return None

    expected = TOOL_VERSIONS.get(name)
    if expected is None:
        return path

    output = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [path, "--version"], check=False, capture_output=True, text=True
    ).stdout
    found = output.split("version ")[1].split()[0] if "version " in output else "unknown"
    if found != expected:
        print_notice("Error", f"{name} {found} at {path}, but {expected} is required", Style.RED)
        return None

    return path


def run(command: list[str], log: Path | None = None) -> bool:
    if log is None:
        sys.stdout.flush()
        return subprocess.run(command, cwd=ROOT_DIR, check=False).returncode == 0  # noqa: S603 - fixed argv

    with log.open("a") as handle:
        handle.write(f"$ {' '.join(command)}\n")
        handle.flush()
        result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
            command,
            cwd=ROOT_DIR,
            check=False,
            stdout=handle,
            stderr=subprocess.STDOUT,
        )
    return result.returncode == 0


# -----------------------------------------------------------------------------
# Project discovery
# -----------------------------------------------------------------------------


def is_project_path(path: Path) -> bool:
    return any(path.is_relative_to(ROOT_DIR / directory) for directory in SOURCE_DIRS)


def project_files(suffixes: tuple[str, ...] = HEADER_SUFFIXES + SOURCE_SUFFIXES) -> list[Path]:
    return sorted(
        path
        for directory in SOURCE_DIRS
        for path in (ROOT_DIR / directory).rglob("*")
        if path.suffix in suffixes and path.is_file() and "build" not in path.relative_to(ROOT_DIR).parts
    )


def configure_presets() -> list[str]:
    presets = json.loads((ROOT_DIR / "CMakePresets.json").read_text())
    return [
        preset["name"]
        for preset in presets["configurePresets"]
        if not preset.get("hidden", False) and preset["name"] not in ALL_EXCLUDED_PRESETS
    ]


def preset_directory(preset: str) -> Path:
    return BUILD_DIR / preset


def executable_path(directory: Path, name: str) -> Path:
    return directory / (f"{name}.exe" if os.name == "nt" else name)


# -----------------------------------------------------------------------------
# Steps (each reports its own result and returns whether it passed)
# -----------------------------------------------------------------------------


def configure_and_build(preset: str, jobs: int, warnings_as_errors: bool, fresh: bool, log: Path | None = None) -> bool:
    if fresh:
        shutil.rmtree(preset_directory(preset), ignore_errors=True)

    print_step(f"[{preset}] Configuring and building with {jobs} job(s)")
    werror = "ON" if warnings_as_errors else "OFF"
    built = run(
        ["cmake", "--preset", preset, "--log-level=WARNING", f"-DTRIVIAL_WARNINGS_AS_ERRORS={werror}"], log
    ) and run(["cmake", "--build", "--preset", preset, "-j", str(jobs)], log)

    print_result(built, "build" + ("" if built or log is None else f", see {log.relative_to(ROOT_DIR)}"))
    return built


def refresh_compile_commands_link(preset: str) -> None:
    target = Path("build") / preset / "compile_commands.json"
    link = ROOT_DIR / "compile_commands.json"

    if not (ROOT_DIR / target).is_file():
        print_notice("Warning", f"{target} not found, leaving compile_commands.json untouched", Style.YELLOW)
        return

    if link.is_symlink() or link.exists():
        link.unlink()

    try:
        link.symlink_to(target)
    except OSError:
        # Most likely Windows without symlink privilege (Developer Mode/elevation)
        shutil.copyfile(ROOT_DIR / target, link)


def format_files(check_only: bool) -> bool:
    clang_format = require_tool("clang-format")
    if clang_format is None:
        return False

    files = [str(path) for path in project_files()]

    if not check_only:
        print_step(f"Formatting {len(files)} file(s)")
        formatted = run([clang_format, "-i", *files])
        print_result(formatted, "format")
        return formatted

    print_step(f"Checking formatting of {len(files)} file(s)")
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [clang_format, "--dry-run", "-Werror", *files],
        cwd=ROOT_DIR,
        check=False,
        capture_output=True,
        text=True,
    )
    # Split from the right so Windows drive letters (C:\\...) stay in the path
    unformatted = sorted(
        {line.split(": error: ")[0].rsplit(":", 2)[0] for line in result.stderr.splitlines() if ": error: " in line}
    )
    for path in unformatted:
        print(f"    {Path(path).relative_to(ROOT_DIR)}")

    print_result(not unformatted, f"{len(unformatted)} file(s) need formatting")
    return not unformatted


def expected_include_guard(header: Path) -> str | None:
    relative = header.relative_to(ROOT_DIR).as_posix()
    if relative.startswith("include/trivial/"):
        name = "TRIVIAL_" + relative.removeprefix("include/trivial/")
    elif relative.startswith("src/"):
        name = "TRIVIAL_SRC_" + relative.removeprefix("src/")
    else:
        return None
    return "".join(character if character.isalnum() else "_" for character in name).upper()


def include_guard_problem(header: Path, guard: str) -> str | None:
    lines = [line.rstrip() for line in header.read_text().splitlines() if line.strip()]
    if len(lines) < 3:
        return "too short for an include guard"
    if lines[0] != f"#ifndef {guard}":
        return f"first line should be '#ifndef {guard}', found '{lines[0]}'"
    if lines[1] != f"#define {guard}":
        return f"second line should be '#define {guard}', found '{lines[1]}'"
    if lines[-1] != f"#endif // {guard}":
        return f"last line should be '#endif // {guard}', found '{lines[-1]}'"
    return None


def check_include_guards() -> bool:
    headers = project_files(HEADER_SUFFIXES)
    print_step(f"Checking include guards of {len(headers)} header(s)")

    problems: list[str] = []
    for header in headers:
        guard = expected_include_guard(header)
        if guard is None:
            continue
        problem = include_guard_problem(header, guard)
        if problem is not None:
            problems.append(f"{header.relative_to(ROOT_DIR)}: {problem}")

    for problem in problems:
        print(f"    {problem}")

    print_result(not problems, f"{len(problems)} header(s) with a wrong include guard")
    return not problems


def tidy_extra_args() -> list[str]:
    if sys.platform != "darwin":
        return []
    xcrun = require_tool("xcrun")
    if xcrun is None:
        return []
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [xcrun, "--show-sdk-path"],
        check=True,
        capture_output=True,
        text=True,
    )
    return [f"--extra-arg=-isysroot{result.stdout.strip()}"]


def parse_diagnostic(line: str) -> DiagnosticKey | None:
    for level in ("warning", "error"):
        location, separator, message = line.partition(f": {level}: ")
        if not separator:
            continue
        # Split from the right so Windows drive letters (C:\\...) stay in the path
        parts = location.rsplit(":", 2)
        if len(parts) != 3 or not parts[1].isdigit() or not parts[2].isdigit():
            return None
        path = str(Path(parts[0]).resolve())
        return DiagnosticKey(path, int(parts[1]), int(parts[2]), level, message)
    return None


def format_diagnostic(key: DiagnosticKey) -> str:
    return f"{key.path}:{key.line}:{key.column}: {key.level}: {key.message}"


def run_tidy(directory: Path, jobs: int, indent: str = "") -> bool:
    clang_tidy = require_tool("clang-tidy")
    if clang_tidy is None:
        return False

    compile_commands = directory / "compile_commands.json"
    if not compile_commands.is_file():
        print_result(False, f"clang-tidy: {compile_commands.relative_to(ROOT_DIR)} does not exist", f"{indent}    ")
        return False

    commands = json.loads(compile_commands.read_text())
    sources = sorted({path for entry in commands if is_project_path(path := Path(entry["file"]).resolve())})
    headers = project_files(HEADER_SUFFIXES)
    extra_args = tidy_extra_args()

    def tidy_file(source: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
            [clang_tidy, "-p", str(directory), "--quiet", *extra_args, str(source)],
            cwd=ROOT_DIR,
            check=False,
            capture_output=True,
            text=True,
        )

    print_step(f"Running clang-tidy over {len(sources)} source(s) and {len(headers)} header(s)", indent)
    with ThreadPoolExecutor(jobs) as pool:
        results = list(pool.map(tidy_file, [*sources, *headers]))

    config_errors = sorted(
        {line for result in results for line in result.stderr.splitlines() if "clang-tidy-config" in line}
    )
    for line in config_errors:
        print(f"{indent}    {paint(line, Style.RED, sys.stderr)}", file=sys.stderr)

    # A header reached through several translation units reports the same
    # diagnostic each time, and on Windows its path can be spelt differently
    # each time
    diagnostics: dict[DiagnosticKey, list[str]] = {}
    for result in results:
        key: DiagnosticKey | None = None
        for line in result.stdout.splitlines():
            if (headline := parse_diagnostic(line)) is not None:
                key = headline
                if key in diagnostics:
                    key = None
                else:
                    diagnostics[key] = [format_diagnostic(key)]
            elif key is not None:
                diagnostics[key].append(line)

    report = directory / "clang-tidy.txt"
    report.write_text("".join(f"{line}\n" for key in sorted(diagnostics) for line in diagnostics[key]))

    errors = sum(1 for key in diagnostics if key.level == "error")
    warnings = len(diagnostics) - errors

    passed = not diagnostics and not config_errors
    print_result(
        passed,
        f"clang-tidy: {warnings} warning(s), {errors} error(s), see {report.relative_to(ROOT_DIR)}",
        f"{indent}    ",
    )
    return passed


def run_gtest(binary: Path, gtest_filter: str, report: Path) -> TestRun:
    result = subprocess.run(  # noqa: S603 - fixed argv, no shell, no untrusted input
        [str(binary), f"--gtest_filter={gtest_filter}", "--gtest_brief=1", f"--gtest_output=json:{report}"],
        cwd=ROOT_DIR,
        check=False,
        capture_output=True,
        text=True,
    )

    output = result.stdout + result.stderr
    report.with_suffix(".log").write_text(output)

    # A crash or fatal assertion exits before gtest writes its report
    run = TestRun(result.returncode, output, reported=report.is_file())
    if not run.reported:
        return run

    for suite in json.loads(report.read_text())["testsuites"]:
        for case in suite["testsuite"]:
            if case["status"] != "RUN":
                continue
            run.ran += 1
            if "failures" in case:
                run.failures[f"{suite['name']}.{case['name']}"] = [entry["failure"] for entry in case["failures"]]
            elif case.get("result") == "SKIPPED":
                run.skipped += 1

    return run


def print_test_failures(run: TestRun, label: str, indent: str) -> None:
    if not run.reported:
        print(run.output)
        print_result(False, f"{label} crashed before reporting", indent)
        return

    if run.exit_code != 0 and not run.failures:
        print(run.output)
        print_result(False, f"{label} exited with code {run.exit_code}", indent)
        return

    project_prefixes = (f"{ROOT_DIR.as_posix()}/", f"{ROOT_DIR}{os.sep}")
    for name, messages in run.failures.items():
        print_result(False, name, indent)
        for message in messages:
            for prefix in project_prefixes:
                message = message.replace(prefix, "")
            for line in message.splitlines():
                print(f"{indent}    {line}")


def run_tests(directory: Path, indent: str = "") -> bool:
    binary = executable_path(directory / "tests", "trivial_tests")
    reports = directory / "test-results" / "latest"
    shutil.rmtree(reports, ignore_errors=True)
    reports.mkdir(parents=True)
    result_indent = f"{indent}    "

    print_step("Running shared-process tests", indent)
    shared = run_gtest(binary, "-" + ":".join(ISOLATED_TESTS), reports / "shared.json")
    print_test_failures(shared, "shared-process tests", result_indent)
    if shared.reported:
        skipped = f", {shared.skipped} skipped" if shared.skipped else ""
        passed_count = shared.ran - len(shared.failures)
        print_result(shared.passed, f"{passed_count}/{shared.ran} tests passed{skipped}", result_indent)

    print_step("Running isolated tests", indent)
    isolated_passed = 0
    isolated_skipped = 0
    for test in ISOLATED_TESTS:
        isolated = run_gtest(binary, test, reports / f"{test}.json")
        if isolated.reported and isolated.ran == 0:
            print_result(False, f"{test} (no such test, remove it from ISOLATED_TESTS)", result_indent)
        elif isolated.passed:
            isolated_passed += 1
            isolated_skipped += isolated.skipped
        else:
            print_test_failures(isolated, test, result_indent)

    all_isolated_passed = isolated_passed == len(ISOLATED_TESTS)
    skipped = f", {isolated_skipped} skipped" if isolated_skipped else ""
    print_result(all_isolated_passed, f"{isolated_passed}/{len(ISOLATED_TESTS)} tests passed{skipped}", result_indent)

    passed = shared.passed and all_isolated_passed
    if passed:
        print(f"{result_indent}Reports: {reports.relative_to(ROOT_DIR)}")
    else:
        print(f"{result_indent}Reports kept: {keep_failed_reports(reports, directory.name).relative_to(ROOT_DIR)}")

    return passed


def keep_failed_reports(reports: Path, preset: str) -> Path:
    # Outside the preset directory so fresh builds from --all leave them alone
    timestamp = time.strftime("%Y-%m-%dT%H-%M-%S")
    destination = BUILD_DIR / "test-failures" / preset / timestamp
    suffix = 1
    while destination.exists():
        suffix += 1
        destination = destination.with_name(f"{timestamp} ({suffix})")

    shutil.copytree(reports, destination)
    return destination


def run_sandbox(directory: Path, smoke_seconds: float | None = None, log: Path | None = None, indent: str = "") -> bool:
    binary = executable_path(directory / "sandbox", "trivial_sandbox")

    if smoke_seconds is None:
        print_step("Running sandbox")
        return run([str(binary)])

    print_step(f"Running sandbox for {smoke_seconds:g}s", indent)
    with open(log or os.devnull, "a") as handle:
        process = subprocess.Popen(  # noqa: S603 - fixed argv, no shell, no untrusted input
            [str(binary)],
            cwd=ROOT_DIR,
            stdout=subprocess.DEVNULL,
            stderr=handle,
        )
        try:
            passed = process.wait(timeout=smoke_seconds) == 0
        except subprocess.TimeoutExpired:
            process.terminate()
            process.wait()
            passed = True

    message = "sandbox" + ("" if passed or log is None else f", see {log.relative_to(ROOT_DIR)}")
    print_result(passed, message, f"{indent}    ")
    return passed


# -----------------------------------------------------------------------------
# Pipelines
# -----------------------------------------------------------------------------


def run_preset(args: argparse.Namespace) -> bool:
    passed = True

    if args.format or args.format_check:
        passed &= format_files(check_only=not args.format)
        passed &= check_include_guards()

    if not configure_and_build(args.preset, args.jobs, args.warnings_as_errors, fresh=False):
        return False

    refresh_compile_commands_link(args.preset)
    directory = preset_directory(args.preset)

    if args.tidy:
        passed &= run_tidy(directory, args.jobs)

    if args.test:
        failed_runs = 0
        for run_index in range(args.repeat):
            if args.repeat > 1:
                print()
                print_step(f"Test run {run_index + 1}/{args.repeat}")
            if not run_tests(directory, "    " if args.repeat > 1 else ""):
                failed_runs += 1

        if args.repeat > 1:
            print()
            print_result(failed_runs == 0, f"{args.repeat - failed_runs}/{args.repeat} runs passed", "")
        passed &= failed_runs == 0

    if args.run_sandbox:
        passed &= run_sandbox(directory)

    return passed


def run_all(jobs: int) -> bool:
    started = time.monotonic()
    log_dir = BUILD_DIR / "all-presets"
    shutil.rmtree(log_dir, ignore_errors=True)
    log_dir.mkdir(parents=True)

    format_passed = format_files(check_only=True)
    guards_passed = check_include_guards()

    steps: tuple[str, ...] = ("build", "tests", "sandbox", "tidy")
    results: dict[str, dict[str, Status]] = {}

    for preset in configure_presets():
        print()
        directory = preset_directory(preset)
        log = log_dir / f"{preset}.log"
        row: dict[str, Status] = dict.fromkeys(steps, Status.NOT_RUN)
        results[preset] = row

        row["build"] = Status.of(configure_and_build(preset, jobs, warnings_as_errors=True, fresh=True, log=log))
        if row["build"] is Status.FAILED:
            continue

        row["tests"] = Status.of(run_tests(directory, indent="    "))
        row["sandbox"] = Status.of(run_sandbox(directory, SANDBOX_SMOKE_SECONDS, log, indent="    "))
        row["tidy"] = Status.of(run_tidy(directory, jobs, indent="    "))

    refresh_compile_commands_link("debug")

    print()
    print_step(f"All presets ({time.monotonic() - started:.0f}s)")
    print_table(
        ["check", "result"],
        [["format", Status.of(format_passed)], ["guards", Status.of(guards_passed)]],
    )
    print()
    print_table(["preset", *steps], [[preset, *(row[step] for step in steps)] for preset, row in results.items()])

    return (
        format_passed
        and guards_passed
        and all(status is Status.PASSED for row in results.values() for status in row.values())
    )


# -----------------------------------------------------------------------------
# Command line
# -----------------------------------------------------------------------------


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-p", "--preset", default="debug", help="CMake preset to configure/build (default: %(default)s)"
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=os.cpu_count() or 1,
        help="Parallel compile and clang-tidy jobs (default: all logical cores)",
    )
    parser.add_argument(
        "-w", "--warnings-as-errors", action="store_true", help="Configure with TRIVIAL_WARNINGS_AS_ERRORS=ON"
    )
    parser.add_argument("-t", "--test", action="store_true", help="Run the test suite after building")
    parser.add_argument(
        "-n", "--repeat", type=int, default=1, help="Run the test suite this many times (for stress-testing flakiness)"
    )
    parser.add_argument("-r", "--run-sandbox", action="store_true", help="Run the sandbox after building")
    parser.add_argument(
        "-f", "--format", action="store_true", help="Apply clang-format and check include guards before building"
    )
    parser.add_argument(
        "--format-check",
        action="store_true",
        help="Report unformatted sources and wrong include guards, without changing anything",
    )
    parser.add_argument(
        "--tidy", action="store_true", help="Run clang-tidy over all sources after building (list in build/<preset>)"
    )
    parser.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="Pre-push check: format and include guard checks, then for every preset a fresh build with warnings as"
        " errors, tests, a sandbox run and clang-tidy",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    enable_windows_colour()
    passed = run_all(args.jobs) if args.all else run_preset(args)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
