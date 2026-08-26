import subprocess
import time
import json
from pathlib import Path
from typing import Literal, TypedDict
from statistics import median
import xml.etree.ElementTree as ET

from mcp.server import MCPServer

mcp = MCPServer("agentic-lowlatency-lab")

PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"
TEST_RESULTS_FILE = BUILD_DIR / "mcp-test-results.xml"
BENCHMARK_EXECUTABLE = BUILD_DIR / "order_book_benchmark"
BASELINE_FILE = (
        PROJECT_ROOT
        / "benchmarks"
        / "baseline.json"
)

class BuildResult(TypedDict):
    success: bool
    exit_code: int
    stdout: str
    stderr: str
    duration_ms: int


@mcp.tool()
def build_project() -> BuildResult:
    """Build the current C++ project.
    Use this tool after changing C++ source files to verify that the
    repository still compiles. It uses the existing CMake build directory
    and does not configure or clean the project.
    """

    if not BUILD_DIR.exists():
        return BuildResult(
            success=False,
            exit_code=-1,
            stdout="",
            stderr=(
                f"Build directory does not exist: {BUILD_DIR}. "
                "Configure the project first."
            ),
            duration_ms=0,
        )

    start = time.monotonic()

    try:
        result = subprocess.run(
            [
                "cmake",
                "--build",
                str(BUILD_DIR),
                "--parallel",
            ],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )

    except subprocess.TimeoutExpired:
        duration_ms = int((time.monotonic() - start) * 1000)

        return BuildResult(
            success=False,
            exit_code=-1,
            stdout="",
            stderr="Build timed out after 120 seconds.",
            duration_ms=duration_ms,
        )

    duration_ms = int((time.monotonic() - start) * 1000)

    return BuildResult(
        success=result.returncode == 0,
        exit_code=result.returncode,
        stdout=result.stdout,
        stderr=result.stderr,
        duration_ms=duration_ms,
    )

@mcp.tool()
def get_project_info() -> dict[str, str]:
    """Return basic information about the C++ project."""

    return {
        "project": "agentic-lowlatency-lab",
        "language": "C++23",
        "build_system": "CMake",
        "tests": "CTest",
        "benchmark": "market_data_benchmark",
        "purpose": "Low-latency synthetic market-data processing demo",
    }

class TestCaseResult(TypedDict):
    name: str
    status: Literal["passed", "failed", "skipped"]
    duration_ms: int
    message: str


class TestResult(TypedDict):
    success: bool
    total: int
    passed: int
    failed: int
    skipped: int
    tests: list[TestCaseResult]
    stdout: str
    stderr: str
    duration_ms: int

def parse_test_results(path: Path) -> list[TestCaseResult]:
    tree = ET.parse(path)
    root = tree.getroot()

    tests: list[TestCaseResult] = []

    for testcase in root.iter("testcase"):
        name = testcase.attrib.get("name", "unknown")

        time_seconds = float(
            testcase.attrib.get("time", "0")
        )

        failure = testcase.find("failure")
        skipped = testcase.find("skipped")

        if failure is not None:
            status = "failed"
            message = (
                    failure.attrib.get("message")
                    or failure.text
                    or ""
            ).strip()

        elif skipped is not None:
            status = "skipped"
            message = ""

        else:
            status = "passed"
            message = ""

        tests.append(
            TestCaseResult(
                name=name,
                status=status,
                duration_ms=int(time_seconds * 1000),
                message=message,
            )
        )

    return tests

@mcp.tool()
def run_tests() -> TestResult:
    """Run the project's CTest test suite.

    Use this tool after a successful build to verify that changes
    preserve expected behavior.

    The result contains structured information about passed,
    failed and skipped tests.
    """

    if not BUILD_DIR.exists():
        return TestResult(
            success=False,
            total=0,
            passed=0,
            failed=0,
            skipped=0,
            tests=[],
            stdout="",
            stderr=f"Build directory does not exist: {BUILD_DIR}",
            duration_ms=0,
        )

    if TEST_RESULTS_FILE.exists():
        TEST_RESULTS_FILE.unlink()

    start = time.monotonic()

    try:
        result = subprocess.run(
            [
                "ctest",
                "--test-dir",
                str(BUILD_DIR),
                "--output-on-failure",
                "--no-tests=error",
                "--output-junit",
                str(TEST_RESULTS_FILE),
            ],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )

    except subprocess.TimeoutExpired:
        duration_ms = int(
            (time.monotonic() - start) * 1000
        )

        return TestResult(
            success=False,
            total=0,
            passed=0,
            failed=0,
            skipped=0,
            tests=[],
            stdout="",
            stderr="Tests timed out after 120 seconds.",
            duration_ms=duration_ms,
        )

    duration_ms = int(
        (time.monotonic() - start) * 1000
    )

    tests: list[TestCaseResult] = []

    if TEST_RESULTS_FILE.exists():
        try:
            tests = parse_test_results(TEST_RESULTS_FILE)
        except ET.ParseError as error:
            return TestResult(
                success=False,
                total=0,
                passed=0,
                failed=0,
                skipped=0,
                tests=[],
                stdout=result.stdout,
                stderr=(
                        result.stderr
                        + f"\nFailed to parse JUnit XML: {error}"
                ),
                duration_ms=duration_ms,
            )

    passed = sum(
        test["status"] == "passed"
        for test in tests
    )

    failed = sum(
        test["status"] == "failed"
        for test in tests
    )

    skipped = sum(
        test["status"] == "skipped"
        for test in tests
    )

    return TestResult(
        success=result.returncode == 0 and failed == 0,
        total=len(tests),
        passed=passed,
        failed=failed,
        skipped=skipped,
        tests=tests,
        stdout=result.stdout,
        stderr=result.stderr,
        duration_ms=duration_ms,
    )

class BenchmarkMetrics(TypedDict):
    events: int
    applied_events: int
    rejected_events: int
    rejected_percent: float
    throughput_events_per_sec: float
    avg_ns: float
    p50_ns: int
    p99_ns: int
    p999_ns: int

class BenchmarkResult(TypedDict):
    success: bool
    metrics: BenchmarkMetrics
    duration_ms: int
    stdout: str
    stderr: str

def _run_benchmark_once() -> BenchmarkResult:

    if not BENCHMARK_EXECUTABLE.exists():
        return BenchmarkResult(
            success=False,
            metrics=BenchmarkMetrics(
                events=0,
                applied_events=0,
                rejected_events=0,
                rejected_percent=0.0,
                throughput_events_per_sec=0.0,
                avg_ns=0.0,
                p50_ns=0,
                p99_ns=0,
                p999_ns=0),
            duration_ms=0,
            stdout="",
            stderr=(
                f"Benchmark executable does not exist: "
                f"{BENCHMARK_EXECUTABLE}"
            ),
        )

    start = time.monotonic()

    try:
        result = subprocess.run(
            [
                str(BENCHMARK_EXECUTABLE),
                "--json",
            ],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )

    except subprocess.TimeoutExpired:
        duration_ms = int(
            (time.monotonic() - start) * 1000
        )

        return BenchmarkResult(
            success=False,
            metrics=BenchmarkMetrics(
                events=0,
                applied_events=0,
                rejected_events=0,
                rejected_percent=0.0,
                throughput_events_per_sec=0.0,
                avg_ns=0.0,
                p50_ns=0,
                p99_ns=0,
                p999_ns=0),
            duration_ms=duration_ms,
            stdout="",
            stderr="Benchmark timed out after 120 seconds.",
        )

    duration_ms = int(
        (time.monotonic() - start) * 1000
    )

    if result.returncode != 0:
        return BenchmarkResult(
            success=False,
            metrics=BenchmarkMetrics(
                events=0,
                applied_events=0,
                rejected_events=0,
                rejected_percent=0.0,
                throughput_events_per_sec=0.0,
                avg_ns=0.0,
                p50_ns=0,
                p99_ns=0,
                p999_ns=0),
            duration_ms=duration_ms,
            stdout=result.stdout,
            stderr=result.stderr,
        )

    try:
        data = json.loads(result.stdout)

    except json.JSONDecodeError as error:
        return BenchmarkResult(
            success=False,
            metrics=BenchmarkMetrics(
                events=0,
                applied_events=0,
                rejected_events=0,
                rejected_percent=0.0,
                throughput_events_per_sec=0.0,
                avg_ns=0.0,
                p50_ns=0,
                p99_ns=0,
                p999_ns=0),
            duration_ms=duration_ms,
            stdout=result.stdout,
            stderr=f"Failed to parse benchmark JSON: {error}",
        )

    return BenchmarkResult(
        success=True,
        metrics=BenchmarkMetrics(
            events=int(data["events"]),
            applied_events=int(data["applied_events"]),
            rejected_events=int(data["rejected_events"]),
            rejected_percent=float(data["rejected_percent"]),
            throughput_events_per_sec=float(data["throughput_events_per_sec"]),
            avg_ns=float(data["avg_ns"]),
            p50_ns=int(data["p50_ns"]),
            p99_ns=int(data["p99_ns"]),
            p999_ns=int(data["p999_ns"])),
        duration_ms=duration_ms,
        stdout=result.stdout,
        stderr=result.stderr,
    )

@mcp.tool()
def run_benchmark() -> BenchmarkResult:
    """Run the project's low-latency market-data benchmark.

    Use this tool after a successful build and test run to measure
    current throughput and latency.

    The benchmark uses a deterministic synthetic event stream.
    """
    return _run_benchmark_once()

BENCHMARK_RUNS = 5
BENCHMARK_CONFIG = {
    "event_count": 1_000_000,
    "seed": 42,
    "sample_every": 100,
}

def _run_benchmark_multiple(
        runs: int = BENCHMARK_RUNS,
) -> list[BenchmarkMetrics]:

    results: list[BenchmarkMetrics] = []

    for _ in range(runs):
        result = _run_benchmark_once()

        if not result["success"]:
            raise RuntimeError(
                f"Benchmark failed: {result['stderr']}"
            )

        results.append({
            "events": result["metrics"]["events"],
            "applied_events": result["metrics"]["applied_events"],
            "rejected_events": result["metrics"]["rejected_events"],
            "rejected_percent": result["metrics"]["rejected_percent"],
            "throughput_events_per_sec":
                result["metrics"]["throughput_events_per_sec"],
            "avg_ns": result["metrics"]["avg_ns"],
            "p50_ns": result["metrics"]["p50_ns"],
            "p99_ns": result["metrics"]["p99_ns"],
            "p999_ns": result["metrics"]["p999_ns"],
        })

    return results

def _median_metrics(
        runs: list[BenchmarkMetrics],
) -> BenchmarkMetrics:

    return {
        "events": runs[0]["events"],
        "applied_events": int(
            median(r["applied_events"] for r in runs)
        ),
        "rejected_events": int(
            median(r["rejected_events"] for r in runs)
        ),
        "rejected_percent": median(
            r["rejected_percent"] for r in runs
        ),
        "throughput_events_per_sec": median(
            r["throughput_events_per_sec"] for r in runs
        ),
        "avg_ns": median(
            r["avg_ns"] for r in runs
        ),
        "p50_ns": int(
            median(r["p50_ns"] for r in runs)
        ),
        "p99_ns": int(
            median(r["p99_ns"] for r in runs)
        ),
        "p999_ns": int(
            median(r["p999_ns"] for r in runs)
        ),
    }

def _git_commit() -> str:
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
        check=True,
    )

    return result.stdout.strip()

@mcp.tool()
def save_benchmark_baseline() -> dict:
    """Create a benchmark baseline from 5 runs.

    Use only when the current implementation is known to be a
    valid reference version.

    The benchmark must use the standard workload and Release build.
    """

    runs = _run_benchmark_multiple(5)
    metrics = _median_metrics(runs)

    baseline = {
        "benchmark": "order_book_benchmark",
        "git_commit": _git_commit(),
        "runs": len(runs),
        **BENCHMARK_CONFIG,
        "build_type": "Release",
        "median": metrics,
        "measurements": runs,
    }

    BASELINE_FILE.write_text(
        json.dumps(
            baseline,
            indent=2,
        )
        + "\n"
    )

    return baseline

@mcp.tool()
def get_benchmark_baseline() -> dict:
    """Return the currently saved performance baseline."""

    if not BASELINE_FILE.exists():
        return {
            "success": False,
            "error": "Benchmark baseline does not exist."
        }

    return {
        "success": True,
        "baseline": json.loads(
            BASELINE_FILE.read_text()
        ),
    }

class BenchmarkChanges(TypedDict):
    throughput_percent: float
    avg_ns_percent: float
    p50_percent: float
    p99_percent: float
    p999_percent: float


class BenchmarkComparison(TypedDict):
    success: bool
    status: Literal["OK", "WARNING", "REGRESSION", "ERROR"]
    baseline: BenchmarkMetrics
    current: BenchmarkMetrics
    changes: BenchmarkChanges
    message: str

def _percent_change(
        baseline: float,
        current: float,
) -> float:

    if baseline == 0:
        return 0.0

    return (
            (current - baseline)
            / baseline
            * 100.0
    )

def _performance_status(
        throughput_change: float,
        p99_change: float,
) -> Literal["OK", "WARNING", "REGRESSION"]:
    # Throughput:
    # negative change means performance got worse.
    if throughput_change <= -10.0:
        return "REGRESSION"

    # Latency:
    # positive change means performance got worse.
    if p99_change >= 20.0:
        return "REGRESSION"

    if throughput_change <= -5.0:
        return "WARNING"

    if p99_change >= 10.0:
        return "WARNING"

    return "OK"

def _empty_metrics() -> BenchmarkMetrics:
    return BenchmarkMetrics(
        events=0,
        applied_events=0,
        rejected_events=0,
        rejected_percent=0.0,
        throughput_events_per_sec=0.0,
        avg_ns=0.0,
        p50_ns=0,
        p99_ns=0,
        p999_ns=0,
    )

def _empty_changes() -> BenchmarkChanges:
    return BenchmarkChanges(
        throughput_percent=0.0,
        avg_ns_percent=0.0,
        p50_percent=0.0,
        p99_percent=0.0,
        p999_percent=0.0,
    )

def _comparison_message(
        status: str,
        changes: BenchmarkChanges,
) -> str:
    return (
        f"{status}: "
        f"throughput {changes['throughput_percent']:+.2f}%, "
        f"avg latency {changes['avg_ns_percent']:+.2f}%, "
        f"p50 {changes['p50_percent']:+.2f}%, "
        f"p99 {changes['p99_percent']:+.2f}%, "
        f"p99.9 {changes['p999_percent']:+.2f}%."
    )

@mcp.tool()
def compare_benchmarks() -> BenchmarkComparison:
    """Compare current benchmark performance against the saved baseline.

    Runs the benchmark multiple times and compares median results
    against the saved baseline.

    Throughput decrease:
      5-10%  -> WARNING
      >10%   -> REGRESSION

    p99 latency increase:
      10-20% -> WARNING
      >20%   -> REGRESSION

    p99.9 is reported but does not independently determine status.
    """

    if not BASELINE_FILE.exists():
        return BenchmarkComparison(
            success=False,
            status="ERROR",
            baseline=_empty_metrics(),
            current=_empty_metrics(),
            changes=_empty_changes(),
            message=(
                f"Benchmark baseline does not exist: "
                f"{BASELINE_FILE}"
            ),
        )

    try:
        baseline_data = json.loads(
            BASELINE_FILE.read_text()
        )
    except (json.JSONDecodeError, OSError) as error:
        return BenchmarkComparison(
            success=False,
            status="ERROR",
            baseline=_empty_metrics(),
            current=_empty_metrics(),
            changes=_empty_changes(),
            message=f"Failed to read benchmark baseline: {error}",
        )

    for key, expected in BENCHMARK_CONFIG.items():
        actual = baseline_data.get(key)
        if actual != expected:
            return BenchmarkComparison(
                success=False,
                status="ERROR",
                baseline=_empty_metrics(),
                current=_empty_metrics(),
                changes=_empty_changes(),
                message=(
                    f"Benchmark condition mismatch for '{key}': "
                    f"baseline={actual}, current={expected}."
                ),
            )

    try:
        baseline: BenchmarkMetrics = baseline_data["median"]
    except KeyError:
        return BenchmarkComparison(
            success=False,
            status="ERROR",
            baseline=_empty_metrics(),
            current=_empty_metrics(),
            changes=_empty_changes(),
            message="Baseline file does not contain median metrics.",
        )

    try:
        runs = _run_benchmark_multiple(BENCHMARK_RUNS)
        current = _median_metrics(runs)
    except RuntimeError as error:
        return BenchmarkComparison(
            success=False,
            status="ERROR",
            baseline=baseline,
            current=_empty_metrics(),
            changes=_empty_changes(),
            message=f"Current benchmark failed: {error}",
        )

    changes = BenchmarkChanges(
        throughput_percent=_percent_change(
            baseline["throughput_events_per_sec"],
            current["throughput_events_per_sec"],
        ),
        avg_ns_percent=_percent_change(
            baseline["avg_ns"],
            current["avg_ns"],
        ),
        p50_percent=_percent_change(
            baseline["p50_ns"],
            current["p50_ns"],
        ),
        p99_percent=_percent_change(
            baseline["p99_ns"],
            current["p99_ns"],
        ),
        p999_percent=_percent_change(
            baseline["p999_ns"],
            current["p999_ns"],
        ),
    )

    status = _performance_status(
        throughput_change=changes["throughput_percent"],
        p99_change=changes["p99_percent"],
    )

    return BenchmarkComparison(
        success=True,
        status=status,
        baseline=baseline,
        current=current,
        changes=changes,
        message=_comparison_message(
            status=status,
            changes=changes,
        ),
    )

if __name__ == "__main__":
    mcp.run()
