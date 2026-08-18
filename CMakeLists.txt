cmake_minimum_required(VERSION 3.20)
project(agentic-lowlatency-lab)
set(CMAKE_CXX_STANDARD 23)

set(SOURCE_FILES src/main.cpp)

add_executable(agentic-lowlatency-lab ${SOURCE_FILES})
