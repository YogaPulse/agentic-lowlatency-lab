---
name: cpp-low-latency-review
description: Review performance-sensitive C++ changes for allocations, copies, lifetime issues, synchronization, cache behavior, algorithmic complexity, and benchmark regressions.
---

# C++ Low-Latency Review

Review the current C++ changes.

Do not modify files unless explicitly requested.

Focus primarily on the current change.

## 1. Inspect the change

Inspect the current git diff and relevant surrounding code.

Determine:

- which files changed;
- which components are affected;
- whether the change affects the measured hot path;
- whether behavior or data structures changed.

## 2. Review correctness

Check correctness, ownership, object lifetime, iterator/reference
invalidation, and unintended behavior changes.

## 3. Review low-latency risks

Read:

references/performance-checklist.md

Use the checklist to inspect only issues relevant to the current change.

Do not report theoretical problems in unchanged code unless they are
necessary to explain a measured regression.

## 4. Validate

Use project-specific MCP tools when available.

Run:

1. build_project()
2. run_tests()
3. compare_benchmarks() for performance-sensitive changes

Do not modify or replace the saved benchmark baseline.

## 5. Report

Group findings by severity:

CRITICAL
HIGH
MEDIUM
LOW

For every finding include:

- file and location;
- evidence;
- problem;
- expected impact;
- suggested minimal fix.

Then report:

- build status;
- test status;
- benchmark comparison status;
- throughput delta;
- p99 delta.

Clearly distinguish measured facts from hypotheses.

Do not modify files during a review-only task.