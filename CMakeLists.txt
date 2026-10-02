cmake_minimum_required(VERSION 3.20)
project(HFT_Engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 1. Core Engine Library
add_library(hft_engine
    src/orderbook.cpp
)

# 2. Main Live Orchestration Executable
add_executable(hft_sim src/main.cpp)
target_link_libraries(hft_sim PRIVATE hft_engine Ws2_32)


# 3. Third-Party Dependencies
include(FetchContent)

# GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# 4. Subdirectories
add_subdirectory(tests)
add_subdirectory(bench)