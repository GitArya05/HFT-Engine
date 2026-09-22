add_executable(hft_tests
    sanity_test.cpp
    types_test.cpp
)
target_link_libraries(hft_tests PRIVATE GTest::gtest_main hft_warnings)
target_include_directories(hft_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

gtest_discover_tests(hft_tests)
