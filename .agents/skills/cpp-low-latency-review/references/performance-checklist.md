# C++ Low-Latency Performance Checklist

Use this checklist only for code relevant to the current change.

Do not report an issue merely because a construct can theoretically be
expensive.

Determine whether it affects a measured, known, or plausibly
performance-sensitive path.

Prefer no finding over a speculative finding.

# Memory Allocation

Check for:

- `new` / `delete` on the hot path;
- `malloc` / `free`;
- `std::make_shared` and `std::make_unique` in frequently executed code;
- `std::string` construction;
- `std::vector` growth or reallocation;
- temporary containers;
- associative-container insertion that may allocate;
- hidden allocations introduced through library calls.

For containers, distinguish between:

- construction;
- `reserve`;
- insertion within existing capacity;
- insertion that may reallocate.

Do not report `std::vector` merely because it can allocate.

Identify whether allocation can actually occur in the reviewed path.

Do not report allocations performed exclusively during initialization,
benchmark setup, or other cold-path work as hot-path findings.

# Copies and Temporaries

Check for:

- large or non-trivial objects passed by value;
- accidental copies caused by missing references;
- unnecessary container copies;
- unnecessary `std::string` copies;
- unnecessary copying of return values;
- temporary objects created inside frequently executed loops;
- repeated conversions between representations.

Pay attention to changes such as:

```cpp
const auto value = ...
```
versus:
```
const auto& value = ...
```
but report them only when a meaningful copy actually occurs.

When reviewing pass-by-value changes on a hot path, distinguish C++
value semantics from actual machine-level cost.

A by-value parameter may introduce additional copying or data movement,
but the actual cost depends on:

- object size;
- triviality;
- ABI;
- compiler optimization;
- inlining;
- calling convention;
- how the callee uses the value.

Do not report pass-by-value as a performance finding merely because C++
value semantics are present.

Report it as a performance finding only when there is additional evidence,
such as:

- the copied object is large or non-trivial;
- copying performs allocation, reference counting, or other observable work;
- the copy scales with container size;
- generated code or profiling shows additional work;
- benchmark evidence shows a repeatable meaningful regression.

Otherwise, it may be mentioned as an observation without assigning
severity.

Do not claim a specific runtime cost without measurement.

# Ownership and Lifetime

Check for:

- dangling std::string_view;
- dangling references or pointers;
- references to temporary objects;
- iterator invalidation;
- pointer or reference invalidation after container growth;
- use-after-move risks;
- ownership changes that add unnecessary shared ownership;
- lifetime changes that introduce additional indirection or synchronization.

Do not recommend std::string_view unless lifetime safety is clear.

Correctness takes priority over performance when an ownership or lifetime
issue affects both.

# Data Layout and Cache Behavior

Check for:

- pointer-heavy structures introduced into hot code;
- excessive pointer chasing;
- fragmented storage;
- unnecessarily large hot-path objects;
- poor locality between frequently accessed fields;
- unnecessary indirection;
- hot and cold data mixed in frequently accessed structures.

Consider contiguous storage only when it is relevant to the changed
workload.

Do not recommend SoA, flat containers, custom allocators, or other
data-layout rewrites without evidence that they address a concrete problem.

Do not claim cache misses or cache-line effects without measurement or
clear structural evidence.

# Algorithms

Check for:

- accidental O(N) work added to a frequently executed operation;
- complexity increasing with book size, container size, or event count;
- nested loops introduced into hot paths;
- repeated lookups;
- repeated parsing;
- repeated sorting;
- redundant scans;
- unnecessary container traversal;
- repeated work that could safely be performed once outside the hot path.

Compare complexity before and after the current change when relevant.

Distinguish asymptotic complexity from measured runtime.

Do not claim that an asymptotic improvement produces a measurable speedup
without measurement.

# Branches and CPU Work

Check for:

- unnecessary branches in frequently executed code;
- duplicated condition checks;
- repeated work that could be moved out of the hot path;
- expensive conversions;
- virtual dispatch introduced into a hot path;
- repeated computations whose result is invariant across iterations.

Do not make claims about:

- branch prediction;
- instruction-cache effects;
- pipeline stalls;
- vectorization;
- generated instruction count;

without appropriate evidence.

# Concurrency

Check for:

- newly introduced mutexes;
- lock contention risks;
- unnecessary atomics;
- atomic operations added to frequently executed paths;
- stronger memory ordering than required;
- false-sharing risks;
- new shared mutable state;
- reference-counted ownership introduced on hot paths.

Do not recommend weaker memory ordering unless correctness can be
demonstrated.

Do not claim contention or false sharing without evidence that the
relevant memory is actually shared across concurrent execution.

# Containers

For each changed container usage, consider:

- allocation behavior;
- lookup complexity;
- insertion and deletion complexity;
- iteration locality;
- iterator, pointer, and reference stability;
- expected number of elements;
- whether the operation copies the complete container;
- whether the container is used on a hot or cold path.

Do not automatically prefer:

- std::unordered_map over std::map;
- std::vector over node-based containers;
- flat containers over standard containers.

Container choice must be justified by workload, correctness requirements,
and measured behavior when performance claims are made.

# Hot-Path Relevance

Before assigning a performance finding, determine:

- whether the changed operation is actually executed on the hot path;
- how frequently it executes;
- whether its cost grows with input or container size;
- whether it occurs inside or outside the measured region;
- whether the current diff introduced the cost.

Do not report existing unrelated performance characteristics unless the
current change materially interacts with them.

Do not treat benchmark setup, event generation, initialization, logging
outside the measured region, or one-time allocation as hot-path work unless
the benchmark or production path actually includes it.

# Benchmark Discipline

When benchmark evidence is available:

- prefer compare_benchmarks() results from a valid baseline comparison;
- ensure baseline and current benchmark conditions are equivalent;
- use the comparison status provided by repository tooling;
- do not independently redefine configured thresholds;
- do not modify the benchmark workload to improve results;
- do not create, replace, or update baseline.json during review;
- do not claim an improvement or regression from a single noisy measurement;
- do not draw benchmark conclusions after build or test failure.

When the primary agent owns shared validation, consume benchmark results
provided by the parent instead of repeating benchmark execution.

When this Skill owns validation directly and a valid baseline exists, use
```compare_benchmarks()``` rather than a single ```run_benchmark()``` result.

If benchmark conditions differ in event count, seed, sampling, measured
operation, build type, executable behavior, or output semantics, treat the
comparison as invalid.

Throughput and average latency are closely related and should not be treated
as fully independent regression signals.

p50 is primarily informational unless repository policy states otherwise.

p99 is the primary tail-latency metric when configured by repository policy.

p99.9 is informational unless repository policy states otherwise.

# Performance Findings

A performance finding should normally have at least one of:

- concrete additional work introduced on a hot path;
- worse algorithmic complexity;
- a required allocation or expensive operation;
- profiling or generated-code evidence;
- a repeatable benchmark regression;
- another concrete mechanism that explains performance risk.

Do not assign severity solely because a construct is commonly considered
slow.

When benchmark status is ```OK```, a code observation may still be reported if
it represents concrete unnecessary hot-path work, but do not describe it as
a measured regression.

When benchmark status is ```WARNING``` or ```REGRESSION```, do not automatically
attribute the result to the most suspicious-looking code change.

# Optimization Suggestions

Prefer the smallest justified change.

Do not recommend architectural optimizations unless they address:

1. a concrete issue introduced by the current change; or
2. a measured and relevant performance problem.

Do not recommend alternative containers, custom allocators, SoA layouts,
branchless implementations, custom synchronization primitives, or major
rewrites without supporting evidence.

# Evidence Discipline

Separate review statements into the following categories.

## Measured fact

A result directly supported by:

- build output;
- test output;
- benchmark comparison;
- profiler data;
- generated-code inspection;
- other deterministic tooling.

Example:

>Throughput decreased by 12% under equivalent benchmark conditions.

## Code evidence

A concrete property visible in the reviewed code.

Example:

>The current diff copies the complete levels vector inside 
> ```OrderBook::apply()```.

## Hypothesis

A suspected causal relationship that requires validation.

Example:

>The newly introduced vector copy is a likely contributor to the measured
throughput regression.

Do not state a hypothesis as proven unless an appropriate experiment or
other strong evidence establishes causality.

A before/after benchmark that recovers after reverting the suspected change
strengthens causal evidence but does not justify claiming an exact cost in
the presence of measurement noise.

# False-Positive Discipline

Before reporting a finding, ask:

- Was this behavior introduced or materially changed by the current diff?
- Is it relevant to a hot or performance-sensitive path?
- Is the claimed cost real rather than merely possible?
- Is there concrete code or measured evidence?
- Is the severity proportional to the demonstrated impact?
- Is the suggested fix smaller and safer than the problem it addresses?

If the answer is unclear, prefer an observation or no finding over a
speculative finding.