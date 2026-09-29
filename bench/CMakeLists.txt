# Only define hft_bench here
add_executable(hft_bench 
    sanity_bench.cpp 
    orderbook_bench.cpp
)

target_link_libraries(hft_bench PRIVATE 
    hft_engine 
    benchmark::benchmark
)

if (MSVC)
    target_compile_options(hft_bench PRIVATE /W4 /O2)
else()
    target_compile_options(hft_bench PRIVATE -Wall -Wextra -O3)
endif()