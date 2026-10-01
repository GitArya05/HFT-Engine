add_executable(hft_concurrent_bench orderbook_concurrent_bench.cpp)

target_link_libraries(hft_concurrent_bench PRIVATE 
    hft_engine 
    benchmark::benchmark
)