cmake_minimum_required(VERSION 3.20)
project(agentic-lowlatency-lab)
set(CMAKE_CXX_STANDARD 23)


include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP 1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(LIBRARY_SOURCE_FILES
    src/instrument_registry.cpp
    src/order_book.cpp
    src/synthetic_feed.cpp
)

add_library(market_data ${LIBRARY_SOURCE_FILES})

add_executable(unit_tests tests/test.cpp)

target_link_libraries(unit_tests market_data GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)


add_executable(agentic-lowlatency-lab src/main.cpp)
target_link_libraries(agentic-lowlatency-lab market_data)

add_executable(order_book_benchmark benchmarks/order_book_benchmark.cpp)
target_link_libraries(order_book_benchmark market_data)
