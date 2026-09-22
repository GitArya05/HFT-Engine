cmake_minimum_required(VERSION 3.28)
project(hft_matching_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HFT_ENABLE_ASAN "Build with AddressSanitizer (+ UBSan off MSVC)" OFF)
option(HFT_BUILD_TESTS "Build the GoogleTest suite" ON)
option(HFT_BUILD_BENCH "Build the Google Benchmark suite" ON)

# ---------------------------------------------------------------------------
# Warnings are load-bearing, not decoration: -Wall -Wextra -Wpedantic (or the
# MSVC equivalent) apply to every target via this interface library.
# ---------------------------------------------------------------------------
add_library(hft_warnings INTERFACE)
if (MSVC)
    target_compile_options(hft_warnings INTERFACE /W4 /permissive- /EHsc)
else()
    target_compile_options(hft_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

if (HFT_ENABLE_ASAN)
    if (MSVC)
        # MSVC's /fsanitize=address covers heap/stack/global overflow.
        # There is no MSVC UBSan; that gap is covered by the Linux CI job.
        #
        # IMPORTANT: this is applied globally via add_compile_options() below,
        # not just to our own targets via hft_warnings. /fsanitize=address
        # adds STL container-overflow annotations to any STL type used in an
        # inline function, which changes that type's ABI - so linking an
        # ASan object against a GoogleTest/Benchmark built WITHOUT ASan fails
        # with LNK2038 "mismatch detected for 'annotate_*'" errors, and MSVC
        # keeps adding new annotated types (vector/string/optional/...) across
        # versions, so chasing individual _DISABLE_*_ANNOTATION macros is
        # fragile. Building every dependency with the same flag is the robust
        # fix, at the cost of also instrumenting GoogleTest/Benchmark in
        # Debug builds.
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# ---------------------------------------------------------------------------
# Third-party test/benchmark deps, pulled at configure time (no Git submodules,
# no vendored copies, no Boost, no third-party allocators/queues).
# ---------------------------------------------------------------------------
include(FetchContent)

if (HFT_BUILD_TESTS)
    # Must match our runtime library setting so MSVC doesn't reject the link
    # (mixing /MD and /MT between gtest and our own targets is a hard error).
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
    enable_testing()
endif()

if (HFT_BUILD_BENCH)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
    )
    FetchContent_MakeAvailable(benchmark)
endif()

add_subdirectory(src)
if (HFT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
if (HFT_BUILD_BENCH)
    add_subdirectory(bench)
endif()
