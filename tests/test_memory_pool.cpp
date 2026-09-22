#include <gtest/gtest.h>
#include <string>
#include "hft/memory_pool.hpp"

// A dummy struct simulating an Order in the order book
struct DummyOrder {
    uint64_t order_id;
    double price;
    uint32_t quantity;

    DummyOrder(uint64_t id, double p, uint32_t q) : order_id(id), price(p), quantity(q) {}
};

class MemoryPoolTest : public ::testing::Test {
protected:
    hft::MemoryPool<DummyOrder> pool{100};  // Capacity of 100 orders
};

TEST_F(MemoryPoolTest, AllocatesAndConstructsCorrectly) {
    DummyOrder* order = pool.allocate(1001, 150.25, 50);

    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->order_id, 1001);
    EXPECT_DOUBLE_EQ(order->price, 150.25);
    EXPECT_EQ(order->quantity, 50);
}

TEST_F(MemoryPoolTest, DeallocatesAndReusesMemory) {
    DummyOrder* order1 = pool.allocate(1, 100.0, 10);
    void* order1_addr = order1;

    pool.deallocate(order1);

    DummyOrder* order2 = pool.allocate(2, 200.0, 20);
    void* order2_addr = order2;

    // The pool should reuse the memory block pushed to the free list
    EXPECT_EQ(order1_addr, order2_addr);
    EXPECT_EQ(order2->order_id, 2);
}

TEST_F(MemoryPoolTest, ThrowsBadAllocWhenExceedingCapacity) {
    hft::MemoryPool<DummyOrder> tiny_pool{2};

    // Allocate maximum capacity
    DummyOrder* o1 = tiny_pool.allocate(1, 10.0, 1);
    DummyOrder* o2 = tiny_pool.allocate(2, 20.0, 2);

    EXPECT_NE(o1, nullptr);
    EXPECT_NE(o2, nullptr);

    // Third allocation should throw
    EXPECT_THROW(tiny_pool.allocate(3, 30.0, 3), std::bad_alloc);
}

TEST_F(MemoryPoolTest, DestructorIsCalledOnDeallocate) {
    struct DestructorCounter {
        int* counter;
        DestructorCounter(int* c) : counter(c) {}
        ~DestructorCounter() {
            (*counter)++;
        }
    };

    hft::MemoryPool<DestructorCounter> dt_pool{5};
    int dt_count = 0;

    DestructorCounter* obj = dt_pool.allocate(&dt_count);
    EXPECT_EQ(dt_count, 0);

    dt_pool.deallocate(obj);
    EXPECT_EQ(dt_count, 1);
}