# Create a static library for the engine core.
# This allows both the main executable and future unit tests to link to the same logic.
add_library(hft_core
    orderbook.cpp
)

# Specify the include directories. 
# PUBLIC ensures that any target linking to hft_core also gets this include path.
target_include_directories(hft_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Enforce C++20 for designated initializers (e.g., .buy_order_id = ...)
target_compile_features(hft_core PUBLIC cxx_std_20)

# Create the primary executable
add_executable(hft_engine
    main.cpp
)

# Link the engine core library to the executable
target_link_libraries(hft_engine PRIVATE hft_core)