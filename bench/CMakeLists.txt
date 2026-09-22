add_executable(hft_bench sanity_bench.cpp)
target_link_libraries(hft_bench PRIVATE benchmark::benchmark hft_warnings)
target_include_directories(hft_bench PRIVATE ${CMAKE_SOURCE_DIR}/include)
