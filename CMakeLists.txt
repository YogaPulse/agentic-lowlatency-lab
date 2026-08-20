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

add_executable(unit_tests tests/test.cpp src/synthetic_feed.cpp)

target_link_libraries(unit_tests GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)


set(SOURCE_FILES src/main.cpp src/synthetic_feed.cpp)

add_executable(agentic-lowlatency-lab ${SOURCE_FILES})
