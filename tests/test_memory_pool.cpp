#include <gtest/gtest.h>
#include "hft/memory_pool.hpp"
#include "hft/order.hpp"

TEST(OrderPoolTest, BasicAllocationDeallocation) {
    hft::OrderPool<1024> pool;
    EXPECT_EQ(pool.capacity(), 1024);
    EXPECT_EQ(pool.available(), 1024);

    hft::Order* o1 = pool.allocate(1, 100, 10, true);
    EXPECT_NE(o1, nullptr);
    EXPECT_EQ(pool.available(), 1023);
    EXPECT_EQ(o1->id, 1);
    EXPECT_EQ(o1->price, 100);

    pool.deallocate(o1);
    EXPECT_EQ(pool.available(), 1024);
}

TEST(OrderPoolTest, PoolExhaustion) {
    hft::OrderPool<2> tiny_pool;

    auto* o1 = tiny_pool.allocate(1, 100, 10, true);
    auto* o2 = tiny_pool.allocate(2, 101, 20, false);

    EXPECT_NE(o1, nullptr);
    EXPECT_NE(o2, nullptr);
    EXPECT_EQ(tiny_pool.available(), 0);

    auto* o3 = tiny_pool.allocate(3, 102, 30, true);
    EXPECT_EQ(o3, nullptr);
}

TEST(OrderPoolTest, ResetRestoresCapacity) {
    hft::OrderPool<10> pool;
    pool.allocate(1, 100, 10, true);
    pool.allocate(2, 101, 20, false);

    EXPECT_EQ(pool.available(), 8);

    pool.reset();
    EXPECT_EQ(pool.available(), 10);
}