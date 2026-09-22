add_executable(hft_tests
    sanity_test.cpp
    types_test.cpp
)

add_executable(hft_tests 
    types_test.cpp 
    test_memory_pool.cpp
)
# (Keep your existing target_link_libraries setup)
target_link_libraries(hft_tests PRIVATE GTest::gtest_main hft_warnings)
target_include_directories(hft_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

gtest_discover_tests(hft_tests)
