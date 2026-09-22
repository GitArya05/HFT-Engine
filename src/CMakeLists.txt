# Stage 0 placeholder target. It exists only to prove the toolchain,
# presets, and build loop work end to end. It becomes the real engine
# entry point starting in Stage 5 (networking); until then it's a smoke test.
add_executable(hft_server main.cpp)
target_link_libraries(hft_server PRIVATE hft_warnings)
target_include_directories(hft_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
