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