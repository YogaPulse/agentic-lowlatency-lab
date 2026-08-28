---
name: cpp-low-latency-review
description: Review performance-sensitive C++ changes for allocations, copies,
  lifetime issues, synchronization, cache behavior and benchmark regressions.
---

# C++ Low-Latency Review

Use this skill to review changes that affect performance-sensitive C++ code.

Do not modify files unless explicitly requested.

## Step 1: Inspect the change

Inspect the current git diff and identify:

- modified C++ files;
- affected components;
- whether the change touches the hot path;
- whether external behavior changed.

Do not assume a performance impact before inspecting the code.

## Step 2: Correctness review

Check for:

- undefined behavior;
- lifetime problems;
- dangling references;
- iterator or reference invalidation;
- ownership mistakes;
- incorrect state transitions;
- accidental behavior changes.

## Step 3: Memory review

Check for:

- heap allocations on the hot path;
- container reallocations;
- unnecessary copies;
- temporary objects;
- string construction;
- shared_ptr reference counting;
- oversized data structures.

## Step 4: CPU and cache review

Check for:

- accidental O(N) operations;
- repeated lookups;
- pointer chasing;
- cache-unfriendly layouts;
- unnecessary branches;
- virtual dispatch;
- redundant parsing or conversions.

## Step 5: Concurrency review

Check for:

- new mutexes;
- unnecessary atomics;
- stronger memory ordering than required;
- contention;
- false-sharing risks.

## Step 6: Validate

Use project-specific MCP tools when available.

1. Build using build_project().
2. If the build succeeds, run tests using run_tests().
3. If tests pass and the change is performance-sensitive, run compare_benchmarks().

Do not modify or replace the saved benchmark baseline.

## Step 7: Report

Return findings grouped by:

CRITICAL
HIGH
MEDIUM
LOW

For each finding include:

- file and location;
- problem;
- why it matters;
- suggested fix.

Then report:

- build status;
- test status;
- performance comparison status;
- throughput delta;
- p99 delta;
- remaining uncertainty.

Clearly distinguish measured facts from hypotheses.

Do not claim that a code change caused a measured regression unless the evidence supports that conclusion.