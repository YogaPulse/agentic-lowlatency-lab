from mcp.server import MCPServer

mcp = MCPServer("agentic-lowlatency-lab")


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
