---
name: cpp-low-latency-review
description: Review performance-sensitive C++ changes for correctness, allocations, copies, lifetime issues, synchronization, cache behavior, algorithmic complexity, and benchmark regressions.
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

Check for:

- undefined behavior;
- ownership mistakes;
- lifetime problems;
- dangling references;
- iterator/reference invalidation;
- incorrect state transitions;
- unintended behavior changes.

## 3. Review low-latency risks

Before completing the review, read:

references/performance-checklist.md

This reference is required for this workflow.

In the final report include:

References used:
- references/performance-checklist.md

Use that checklist only for issues relevant to the current change.

Do not report theoretical or speculative issues merely because a construct
can be expensive.

## 4. Validation workflow

Use project-specific MCP tools when available.

### Build

Run:

build_project()

If the build fails:

- stop validation;
- do not run tests or benchmarks;
- report BUILD FAILED;
- include relevant compiler diagnostics.

### Tests

If the build succeeds, run:

run_tests()

If tests fail:

- stop performance validation;
- do not run compare_benchmarks();
- report TESTS FAILED;
- list failing tests;
- explain likely causes when evidence is available.

### Performance

If:

- build succeeds;
- tests pass;
- the current change is performance-sensitive;

run:

compare_benchmarks()

Do not substitute a single run_benchmark() measurement when a valid
baseline comparison is available.

Do not create, replace, or update the saved baseline.

If no baseline exists:

- report PERFORMANCE NOT VALIDATED;
- explain that baseline comparison was unavailable.

If benchmark conditions do not match:

- report INVALID COMPARISON;
- do not draw performance conclusions.

## 5. Performance status

Treat compare_benchmarks() as the authoritative source for:

- OK;
- WARNING;
- REGRESSION.

Do not independently reinterpret thresholds unless repository policy
explicitly requires it.

A WARNING must be visible in the final report.

An unexpected REGRESSION means the reviewed performance-sensitive change
is not ready for completion.

p99.9 is informational unless repository policy states otherwise.

## 6. Report

Return:

### Summary

- Build: PASS / FAIL
- Tests: PASS / FAIL / NOT RUN
- Performance: OK / WARNING / REGRESSION / NOT RUN / INVALID
- Review result: PASS / NEEDS CHANGES

### Findings

Group findings by:

CRITICAL
HIGH
MEDIUM
LOW

For each finding include:

- file and location;
- evidence;
- problem;
- expected impact;
- suggested minimal fix.

### Validation

Report:

- build result;
- test result;
- baseline comparison status;
- throughput before/after and delta;
- p99 before/after and delta;
- rejected-event information when available.

### Evidence discipline

Clearly distinguish:

- measured facts;
- code evidence;
- hypotheses.

Do not state a suspected cause as proven unless an appropriate before/after
experiment confirms it.

Do not modify files during a review-only task.