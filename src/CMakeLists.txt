# 1. Create a library for the core engine logic (Tests & Benchmarks will link to this)
add_library(hft_engine orderbook.cpp)
target_include_directories(hft_engine PUBLIC ${CMAKE_SOURCE_DIR}/include)

# 2. Create the main executable separately
add_executable(hft_main main.cpp)
target_link_libraries(hft_main PRIVATE hft_engine)