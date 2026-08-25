import subprocess
import time
from pathlib import Path
from typing import TypedDict

from mcp.server import MCPServer

mcp = MCPServer("agentic-lowlatency-lab")

PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"

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



if __name__ == "__main__":
    mcp.run()
