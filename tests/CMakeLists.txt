add_executable(hft_tests 
    types_test.cpp
    test_memory_pool.cpp
    test_ring_buffer.cpp
    test_network.cpp
)

target_link_libraries(hft_tests PRIVATE 
    hft_engine 
    GTest::gtest 
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hft_tests)