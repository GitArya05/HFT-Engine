#include <gtest/gtest.h>

// Stage 0 sanity check: proves GoogleTest is wired up via FetchContent and
// that `ctest` finds and runs it. Replaced by real order-book tests in
// Stage 1 (this file goes away once that suite exists).
TEST(Stage0Sanity, BuildAndTestLoopWorks) {
    EXPECT_EQ(1 + 1, 2);
}
