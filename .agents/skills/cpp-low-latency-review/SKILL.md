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
- whether the change affects a performance-sensitive or measured hot path;
- whether behavior, ownership, lifetime, algorithms, or data structures changed.

## 2. Review correctness

Check correctness issues that are relevant to the changed code, including:

- undefined behavior;
- ownership mistakes;
- lifetime problems;
- dangling references or pointers;
- iterator, pointer, or reference invalidation;
- incorrect state transitions;
- broken data-structure invariants;
- unintended behavior changes.

Concrete correctness issues discovered during performance review should be
reported even when their primary impact is not performance.

Do not expand into an unrelated general correctness review.

## 3. Review low-latency risks

Before completing the review, read:

`references/performance-checklist.md`

This reference is required for this workflow.

Use the checklist only for issues relevant to the current change.

Do not report theoretical or speculative issues merely because a construct
can be expensive in some circumstances.

Establish that an issue is relevant to the changed code and to a
performance-sensitive path before assigning severity.

In the final report include:

References used:

- `references/performance-checklist.md`

## 4. Validation ownership

The Skill supports two validation modes.

### Direct review mode

When this Skill is invoked directly and no parent agent owns shared
validation, use the project-specific MCP validation workflow defined below.

### Delegated review mode

When this Skill is used by a specialized subagent and the parent agent owns
shared build, test, and benchmark validation:

- perform the independent static low-latency review;
- do not independently repeat shared validation;
- do not call `build_project()`, `run_tests()`, `run_benchmark()`, or
  `compare_benchmarks()` unless the parent explicitly delegates validation;
- use validation results supplied by the parent as measured evidence;
- clearly mark validation evidence as unavailable when it has not been
  provided;
- return code evidence, findings, risks, and hypotheses to the parent for
  consolidation.

## 5. Direct validation workflow

Use project-specific MCP tools when available.

### Build

Run:

`build_project()`

If the build fails:

- stop validation;
- do not run tests or benchmarks;
- report Build: FAIL;
- include relevant compiler diagnostics.

### Tests

If the build succeeds, run:

`run_tests()`

If tests fail:

- stop performance validation;
- do not run `compare_benchmarks()`;
- report Tests: FAIL;
- list representative failing tests;
- explain likely causes only when supported by evidence.

### Performance

If:

- the build succeeds;
- tests pass;
- the current change is performance-sensitive;

run:

`compare_benchmarks()`

Do not substitute a single `run_benchmark()` measurement when a valid
baseline comparison is available.

Never create, replace, or update the saved benchmark baseline during review.

If no valid baseline exists:

- report Performance: NOT VALIDATED;
- explain that baseline comparison was unavailable;
- do not create a baseline automatically.

If benchmark conditions do not match:

- report Performance: INVALID;
- do not draw performance conclusions.

## 6. Performance interpretation

Treat `compare_benchmarks()` as the authoritative source for:

- OK;
- WARNING;
- REGRESSION.

Do not independently reinterpret benchmark thresholds unless repository
policy explicitly requires it.

A WARNING must remain visible in the final report.

An unexpected REGRESSION means the reviewed performance-sensitive change
is not ready for completion.

Do not claim that a code change caused a measured performance difference
unless the evidence establishes that causal relationship.

A before/after measurement may support a hypothesis without proving
causality.

p99.9 is informational unless repository policy states otherwise.

## 7. Evidence discipline

Clearly distinguish:

### Measured facts

Results produced by build, tests, benchmarks, profiling, or other
deterministic validation.

### Code evidence

Concrete properties of the reviewed change, such as:

- a container is copied;
- an allocation occurs;
- an algorithm changes complexity;
- a reference may be invalidated;
- synchronization is introduced.

### Hypotheses

Suspected relationships between code evidence and observed behavior that
have not been causally established.

Do not state a suspected cause as proven without sufficient validation.

Prefer no finding over a speculative finding.

## 8. Report

Return a concise report suitable for either the user or a parent agent.

### Summary

Report when the information is available:

- Build: PASS / FAIL / NOT RUN
- Tests: PASS / FAIL / NOT RUN
- Performance: OK / WARNING / REGRESSION / NOT RUN / NOT VALIDATED / INVALID
- Review result: PASS / NEEDS CHANGES

In delegated review mode, do not invent validation statuses that were not
provided by the parent.

### Findings

Group findings by:

- CRITICAL
- HIGH
- MEDIUM
- LOW

For each finding include:

- file and location;
- code or measured evidence;
- problem;
- expected impact;
- suggested minimal fix.

Do not create findings for observations that lack sufficient evidence of a
real problem.

### Validation

When validation results are available, report:

- build result;
- test result;
- baseline comparison status;
- throughput before/after and delta;
- p99 before/after and delta;
- rejected-event information when available.

Do not duplicate large tool logs.

Report only the evidence needed to support the review.

### References

Include:

References used:

- `references/performance-checklist.md`

Do not modify files during a review-only task.
