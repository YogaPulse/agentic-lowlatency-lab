import subprocess
import time
import json
from pathlib import Path
from typing import Literal, TypedDict
import xml.etree.ElementTree as ET

from mcp.server import MCPServer

mcp = MCPServer("agentic-lowlatency-lab")

PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"
TEST_RESULTS_FILE = BUILD_DIR / "mcp-test-results.xml"
BENCHMARK_EXECUTABLE = BUILD_DIR / "order_book_benchmark"

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

class BenchmarkResult(TypedDict):
    success: bool
    events: int
    applied_events: int
    rejected_events: int
    rejected_percent: float
    throughput_events_per_sec: float
    avg_ns: float
    p50_ns: int
    p99_ns: int
    p999_ns: int
    duration_ms: int
    stdout: str
    stderr: str

@mcp.tool()
def run_benchmark() -> BenchmarkResult:
    """Run the project's low-latency market-data benchmark.

    Use this tool after a successful build and test run to measure
    current throughput and latency.

    The benchmark uses a deterministic synthetic event stream.
    """

    if not BENCHMARK_EXECUTABLE.exists():
        return BenchmarkResult(
            success=False,
            events=0,
            applied_events=0,
            rejected_events=0,
            rejected_percent=0.0,
            throughput_events_per_sec=0.0,
            avg_ns=0.0,
            p50_ns=0,
            p99_ns=0,
            p999_ns=0,
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
            events=0,
            applied_events=0,
            rejected_events=0,
            rejected_percent=0.0,
            throughput_events_per_sec=0.0,
            avg_ns=0.0,
            p50_ns=0,
            p99_ns=0,
            p999_ns=0,
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
            events=0,
            applied_events=0,
            rejected_events=0,
            rejected_percent=0.0,
            throughput_events_per_sec=0.0,
            avg_ns=0.0,
            p50_ns=0,
            p99_ns=0,
            p999_ns=0,
            duration_ms=duration_ms,
            stdout=result.stdout,
            stderr=result.stderr,
        )

    try:
        data = json.loads(result.stdout)

    except json.JSONDecodeError as error:
        return BenchmarkResult(
            success=False,
            events=0,
            applied_events=0,
            rejected_events=0,
            rejected_percent=0.0,
            throughput_events_per_sec=0.0,
            avg_ns=0.0,
            p50_ns=0,
            p99_ns=0,
            p999_ns=0,
            duration_ms=duration_ms,
            stdout=result.stdout,
            stderr=f"Failed to parse benchmark JSON: {error}",
        )

    return BenchmarkResult(
        success=True,
        events=int(data["events"]),
        applied_events=int(data["applied_events"]),
        rejected_events=int(data["rejected_events"]),
        rejected_percent=float(data["rejected_percent"]),
        throughput_events_per_sec=float(data["throughput_events_per_sec"]),
        avg_ns=float(data["avg_ns"]),
        p50_ns=int(data["p50_ns"]),
        p99_ns=int(data["p99_ns"]),
        p999_ns=int(data["p999_ns"]),
        duration_ms=duration_ms,
        stdout=result.stdout,
        stderr=result.stderr,
    )

if __name__ == "__main__":
    mcp.run()
