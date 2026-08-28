# Project

This repository contains a C++23 low-latency market-data processing demonstration.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

When project-specific MCP tools are available, prefer `build_project()` over running build commands manually.

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

Performance benchmarks must use a Release build.

When project-specific MCP tools are available:

* use `run_benchmark()` to obtain a single current measurement;
* use `compare_benchmarks()` when evaluating a performance-sensitive change against the saved baseline.

## Repository Structure

* `src/` — market-data types, synthetic feed, instrument registry, and order-book implementation.
* `tests/` — unit tests.
* `benchmarks/` — performance benchmark source and saved baseline data.
* `mcp/` — project-specific MCP tools for build, test, benchmark, and baseline comparison automation.
* `.agents/skills/` — repository-local reusable agent workflows for project-specific engineering tasks.

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
3. Avoid `std::shared_ptr` unless shared ownership is actually required.
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

A claimed performance improvement must be supported by benchmark evidence.

Do not change benchmark methodology merely to produce better numbers.

Unless the task explicitly concerns the benchmark itself, do not change:

* benchmark workload;
* event count;
* synthetic-feed seed;
* latency sampling frequency;
* measured operation;
* benchmark output semantics.

A performance improvement must come from changes to the implementation being measured, not from making the benchmark easier.

# Benchmark Conditions

Baseline and current measurements are comparable only when benchmark conditions are equivalent.

At minimum, verify that the following conditions match:

* benchmark executable;
* Release build configuration;
* event count;
* synthetic-feed seed;
* latency sampling frequency;
* measured operation;
* benchmark output semantics.

If benchmark conditions differ, do not report a performance improvement or regression.

Report the comparison as invalid and explain which conditions differ.

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

Local benchmark results can contain measurement noise. Do not treat very small differences between individual benchmark runs as definitive performance changes.

Benchmark comparisons must use multiple runs and median values.

For regression decisions:

* throughput is the primary throughput metric;
* average nanoseconds per event is reported but is not treated as an independent regression signal because it represents substantially the same information as throughput;
* p99 may participate in regression detection;
* p50 is primarily informational;
* p99.9 is informational and may be substantially noisier than the other metrics.

Do not claim that a change improves performance without benchmark evidence.

# Benchmark Baseline Management

The saved benchmark baseline represents a known-good reference implementation.

Rules:

1. Do not create, replace, or update the benchmark baseline automatically.
2. `save_benchmark_baseline()` may only be used when explicitly requested by a human.
3. Never update the baseline merely because the current implementation shows a regression.
4. Do not modify `benchmarks/baseline.json` manually unless explicitly requested.
5. Prefer creating a baseline from a known-good revision with passing tests.
6. The baseline should record the Git commit when available.
7. Baseline measurements must use multiple benchmark runs and median values.
8. Current comparison measurements must use the same number and type of benchmark runs where practical.
9. If no baseline exists, report that performance comparison cannot be performed.
10. Do not automatically create a baseline to make a comparison possible.

# Regression Policy

Use the following initial thresholds for local benchmark comparisons.

## Throughput

* decrease of less than 5%: normally treated as measurement noise / `OK`;
* decrease from 5% up to 10%: `WARNING`;
* decrease greater than 10%: `REGRESSION`.

## p99 Latency

* increase of less than 10%: normally `OK`;
* increase from 10% up to 20%: `WARNING`;
* increase greater than 20%: `REGRESSION`.

## p99.9 Latency

p99.9 is informational only.

A p99.9 change must not independently change the overall performance status to `WARNING` or `REGRESSION`.

Always report all available benchmark deltas even when the overall status is `OK`.

# Definition of Done

After modifying C++ code:

1. Build the project using `build_project()` when available.
2. If the build fails, report the failure and relevant compiler diagnostics.
3. If the build succeeds, run tests using `run_tests()` when available.
4. Do not consider the task complete while relevant tests are failing.
5. Report failed tests and analyze their cause before considering the task complete.
6. Review the resulting code changes for correctness and unintended modifications.
7. Report:

    * modified files;
    * important implementation decisions;
    * build status;
    * tests executed and their status;
    * remaining correctness risks.

For performance-sensitive changes:

1. Build must succeed.
2. Relevant tests must pass.
3. Run `compare_benchmarks()` against the saved baseline.
4. Do not substitute a single `run_benchmark()` result for baseline comparison when a valid baseline exists.
5. Report:

    * overall performance status;
    * baseline throughput;
    * current throughput;
    * throughput percentage change;
    * baseline average latency;
    * current average latency;
    * p50 before and after;
    * p99 before and after;
    * p99.9 before and after;
    * applied events;
    * rejected events and rejected percentage.
6. If the comparison reports `WARNING`, explicitly mention it in the final result.
7. If the comparison reports an unexpected `REGRESSION`, do not consider the performance-sensitive task complete.
8. Do not resolve a regression by replacing or modifying the saved baseline.
9. If benchmark conditions do not match, report the comparison as invalid rather than drawing performance conclusions.
10. If no baseline exists, report that performance validation against a baseline could not be completed.

A performance improvement may only be claimed when before/after measurements were produced under equivalent benchmark conditions.

# Skills

Use repository Skills for repeatable engineering workflows when applicable.

For performance-sensitive C++ review, prefer the `cpp-low-latency-review` skill.