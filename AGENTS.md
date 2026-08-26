# Project

This repository contains a C++23 low-latency market-data processing demonstration.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

When project-specific MCP tools are available, prefer `build_project()` over running the build command manually.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

When project-specific MCP tools are available, prefer `run_tests()`.

## Benchmarks

```bash
./build/order_book_benchmark
./build/order_book_benchmark --json
```

Performance benchmarks must be run using a Release build.

When project-specific MCP tools are available, prefer `run_benchmark()`.

## Repository Structure

* `src/` — market-data types, synthetic feed, instrument registry, and order-book implementation.
* `tests/` — unit tests.
* `benchmarks/` — performance benchmarks.
* `mcp/` — project-specific MCP tools for build, test, and benchmark automation.

# General Engineering Rules

1. Use C++23.
2. Keep changes focused on the requested task.
3. Do not modify unrelated files.
4. Prefer simple solutions over unnecessary abstractions.
5. Avoid unnecessary dependencies.
6. Preserve existing behavior unless the task explicitly requires changing it.
7. Prefer explicit ownership and clear lifetime semantics.
8. Use non-owning views such as `std::string_view` only when the referenced lifetime is clearly valid.

# Low-Latency C++ Rules

For performance-sensitive code:

1. Avoid heap allocations on the hot path.
2. Avoid unnecessary copies and temporary objects.
3. Avoid `shared_ptr` unless shared ownership is actually required.
4. Avoid virtual dispatch on the hot path unless justified.
5. Do not introduce mutexes without justification.
6. Avoid unnecessary atomics and synchronization.
7. Prefer contiguous and cache-friendly memory layouts.
8. Consider cache locality and pointer chasing.
9. Consider branch predictability.
10. Avoid accidental increases in algorithmic complexity on the hot path.

# Before Modifying Code

For non-trivial changes:

1. Inspect the relevant implementation first.
2. Explain the current behavior and data flow.
3. Identify ownership and lifetime constraints.
4. Identify whether the affected code is performance-sensitive.
5. Propose a focused implementation plan.
6. Identify correctness and performance risks.
7. Only then modify the code.

# Performance Changes

Do not optimize based only on assumptions.

Before making a performance-sensitive change:

1. Identify the suspected bottleneck.
2. Measure the current implementation when a performance claim is involved.
3. Explain the proposed optimization.
4. State the expected performance impact.
5. Only then modify the implementation.

Do not change benchmark methodology merely to produce better numbers.

Unless the task explicitly concerns the benchmark itself, do not change:

* benchmark workload;
* event count;
* synthetic-feed seed;
* latency sampling frequency;
* measured operation;
* benchmark output semantics.

A claimed performance improvement must come from changes to the implementation being measured, not from making the benchmark easier.

# Benchmark Interpretation

The benchmark reports:

* processed events;
* applied events;
* rejected events;
* rejected percentage;
* throughput in events per second;
* average nanoseconds per event;
* p50 latency;
* p99 latency;
* p99.9 latency.

Always report unexpected rejected events.

Do not treat very small differences between benchmark runs as definitive performance changes. Local benchmark results can contain measurement noise.

Do not claim that a change improves performance without benchmark evidence.

p99.9 is a useful diagnostic metric but may be noisier than throughput, average latency, p50, and p99.

# Definition of Done

After modifying C++ code:

1. Build the project using `build_project()` when available.
2. If the build fails, report the failure and relevant compiler diagnostics.
3. If the build succeeds, run tests using `run_tests()` when available.
4. Do not consider the task complete while relevant tests are failing.
5. Report failed tests before making additional corrective changes.
6. For performance-sensitive changes, run `run_benchmark()` after tests pass.
7. Report:

    * modified files;
    * important implementation decisions;
    * build status;
    * tests executed and their status;
    * remaining correctness risks.

For performance-sensitive changes, additionally report:

* throughput;
* average latency;
* p50;
* p99;
* p99.9;
* applied events;
* rejected events and rejected percentage.

If the task claims a performance improvement, compare measurements taken under equivalent benchmark conditions and report the before/after results.
