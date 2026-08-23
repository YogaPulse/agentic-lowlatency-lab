Project

This repository contains a C++23 low-latency market-data processing demonstration.

Build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build -j

Tests

ctest --test-dir build --output-on-failure

Benchmarks

./build/order_book_benchmark

Repository Structure

src/market_data/ — market-data types and processing.

src/order_book/ — order-book implementation.

src/metrics/ — performance metrics.

tests/ — unit tests.

benchmarks/ — performance benchmarks.

General Rules

Use C++23.
Do not modify unrelated files.
Prefer simple solutions.
Avoid unnecessary dependencies.

Low-Latency C++ Rules

Avoid heap allocations on the hot path.
Avoid unnecessary object copies.
Avoid shared_ptr unless ownership requires it.
Avoid virtual dispatch on the hot path.
Do not introduce mutexes without justification.
Prefer contiguous memory layouts.
Consider cache locality.
Consider branch predictability.
Prefer explicit ownership.
Prefer string_view when ownership is not required.
