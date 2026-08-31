# C++ Low-Latency Performance Checklist

Use this checklist only for code relevant to the current change.

Do not report an issue merely because a construct can theoretically
be expensive. Determine whether it affects the measured or likely
hot path.

# Memory Allocation

Check for:

- new/delete on the hot path;
- malloc/free;
- std::make_shared and std::make_unique in frequently executed code;
- std::string construction;
- std::vector growth or reallocation;
- temporary containers;
- map/unordered_map insertion causing allocation;
- hidden allocations introduced through library calls.

For containers, distinguish between:

- construction;
- reserve;
- insertion within capacity;
- insertion that may reallocate.

Do not report std::vector merely because it can allocate.

Identify whether allocation can actually occur in the reviewed path.

# Copies and Temporaries

Check for:

- large objects passed by value;
- accidental copies caused by missing references;
- unnecessary container copies;
- unnecessary std::string copies;
- returning large objects unnecessarily;
- temporary objects created inside loops;
- repeated conversions between representations.

Pay attention to changes such as:

const auto value = ...
vs
const auto& value = ...

but report them only when a meaningful copy actually occurs.

When reviewing pass-by-value changes on a hot path, distinguish C++
value semantics from the actual machine-level cost.

A by-value parameter may introduce additional copying or data movement,
but the actual cost depends on object size, triviality, ABI, compiler
optimization, inlining, and how the callee uses the value.

Do not report pass-by-value as a performance finding merely because C++
value semantics are present.

Report it as a performance finding only when there is additional
evidence, such as:

- the copied object is large or non-trivial;
- copying performs allocation, reference counting, or other observable work;
- generated code or profiling shows additional work;
- benchmark evidence shows a repeatable meaningful regression.

Otherwise, it may be mentioned as an observation without assigning
severity.

Do not claim a specific runtime cost without measurement.

# Ownership and Lifetime

Check for:

- dangling std::string_view;
- dangling references;
- references to temporary objects;
- iterator invalidation;
- pointer invalidation after container growth;
- unclear ownership;
- unnecessary shared ownership;
- use-after-move risks.

Do not recommend std::string_view unless lifetime is clearly safe.

# Data Layout and Cache Behavior

Check for:

- pointer-heavy structures;
- excessive pointer chasing;
- fragmented storage;
- unnecessarily large hot-path objects;
- poor locality between frequently accessed fields;
- unnecessary indirection;
- hot and cold data mixed in the same structure.

Consider whether contiguous storage would improve locality.

Do not recommend a data-layout rewrite without evidence that the
affected structure is relevant to the hot path.

# Algorithms

Check for:

- accidental O(N) work added to a frequently executed operation;
- nested loops;
- repeated lookups;
- repeated parsing;
- repeated sorting;
- redundant scans;
- unnecessary container traversal.

Compare complexity before and after the current change when relevant.

# Branches and CPU Work

Check for:

- unnecessary branches in frequently executed code;
- duplicated condition checks;
- work that could be moved out of the hot path;
- expensive conversions;
- virtual dispatch introduced into the hot path.

Do not make claims about branch prediction without evidence.

# Concurrency

Check for:

- newly introduced mutexes;
- lock contention;
- unnecessary atomics;
- atomic operations added to frequently executed paths;
- stronger memory ordering than required;
- false-sharing risks;
- shared mutable state introduced by the change.

Do not recommend weaker memory ordering unless correctness can be
demonstrated.

# Containers

For each changed container usage, consider:

- allocation behavior;
- lookup complexity;
- insertion complexity;
- iteration locality;
- iterator/reference stability;
- expected number of elements.

Do not automatically prefer unordered_map over map or vector.

Choose based on workload and measured behavior.

# Benchmark Discipline

When reviewing a performance-sensitive change:

- use compare_benchmarks() when a valid baseline exists;
- ensure baseline and current benchmark conditions match;
- do not modify the benchmark workload to improve results;
- do not modify or replace baseline.json;
- do not claim an improvement based on a single noisy measurement.

Throughput and average latency represent closely related information
and should not be treated as independent regression signals.

p99.9 is informational unless repository policy states otherwise.

# Evidence Discipline

Separate findings into:

Measured fact:
A result directly supported by tests, benchmark data, or deterministic
tool output.

Code evidence:
A concrete property visible in the changed code.

Hypothesis:
A suspected causal relationship that requires validation.

Example:

Measured fact:
Throughput decreased by 12%.

Code evidence:
The current diff adds a heap allocation to OrderBook::apply().

Hypothesis:
The allocation is the likely cause of the throughput regression.

Do not state the hypothesis as proven until an appropriate before/after
experiment confirms it.