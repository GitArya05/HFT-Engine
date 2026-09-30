#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include "hft/mpsc_queue.hpp"
#include "hft/ring_buffer.hpp"

// ============================================================================
// SPSC Queue Unit Tests
// ============================================================================

TEST(SPSCQueueTest, BasicPushPop) {
    hft::SPSCQueue<int, 16> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.emplace(42));
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(queue.empty());
}

TEST(SPSCQueueTest, QueueFullAndEmptyBounds) {
    hft::SPSCQueue<int, 4> queue;

    // Fill queue to capacity (4 elements)
    EXPECT_TRUE(queue.emplace(10));
    EXPECT_TRUE(queue.emplace(20));
    EXPECT_TRUE(queue.emplace(30));
    EXPECT_TRUE(queue.emplace(40));

    // Over-filling must fail safely
    EXPECT_FALSE(queue.emplace(50));
    EXPECT_EQ(queue.size(), 4);

    // Pop one element to free space
    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 10);

    // Now space is available for one element
    EXPECT_TRUE(queue.emplace(50));
    EXPECT_FALSE(queue.emplace(60));

    // Drain all remaining elements
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 30);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 40);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 50);

    // Pop from empty queue must fail safely
    EXPECT_FALSE(queue.pop(val));
    EXPECT_TRUE(queue.empty());
}

TEST(SPSCQueueTest, ConcurrentProducerConsumer) {
    constexpr std::size_t NUM_ELEMENTS = 100'000;
    hft::SPSCQueue<std::size_t, 1024> queue;

    // Producer Thread
    std::thread producer([&]() {
        for (std::size_t i = 0; i < NUM_ELEMENTS; ++i) {
            while (!queue.emplace(i)) {
                std::this_thread::yield();  // Spin-wait on queue full
            }
        }
    });

    std::vector<std::size_t> consumed;
    consumed.reserve(NUM_ELEMENTS);

    // Consumer Thread
    std::thread consumer([&]() {
        std::size_t val = 0;
        while (consumed.size() < NUM_ELEMENTS) {
            if (queue.pop(val)) {
                consumed.push_back(val);
            } else {
                std::this_thread::yield();  // Spin-wait on queue empty
            }
        }
    });

    producer.join();
    consumer.join();

    // Verify ordering and complete delivery without loss
    ASSERT_EQ(consumed.size(), NUM_ELEMENTS);
    for (std::size_t i = 0; i < NUM_ELEMENTS; ++i) {
        EXPECT_EQ(consumed[i], i);
    }
}

// ============================================================================
// MPSC Queue Unit Tests
// ============================================================================

TEST(MPSCQueueTest, BasicPushPop) {
    hft::MPSCQueue<int, 16> queue;
    int val = 0;

    EXPECT_FALSE(queue.pop(val));  // Empty
    EXPECT_TRUE(queue.emplace(100));
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 100);
}

TEST(MPSCQueueTest, MultiProducerSingleConsumer) {
    constexpr std::size_t NUM_PRODUCERS = 4;
    constexpr std::size_t ITEMS_PER_PRODUCER = 25'000;
    constexpr std::size_t TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

    hft::MPSCQueue<std::size_t, 4096> queue;
    std::vector<std::thread> producers;
    std::atomic<bool> start_flag{false};

    // Spin up multiple producers
    for (std::size_t p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (std::size_t i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                while (!queue.emplace(p * ITEMS_PER_PRODUCER + i)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::size_t consumed_count = 0;
    std::vector<std::size_t> consumed_items;
    consumed_items.reserve(TOTAL_ITEMS);

    std::thread consumer([&]() {
        while (!start_flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        std::size_t val = 0;
        while (consumed_count < TOTAL_ITEMS) {
            if (queue.pop(val)) {
                consumed_items.push_back(val);
                consumed_count++;
            }
        }
    });

    // Unleash all threads simultaneously
    start_flag.store(true, std::memory_order_release);

    for (auto& prod : producers) {
        prod.join();
    }
    consumer.join();

    EXPECT_EQ(consumed_items.size(), TOTAL_ITEMS);
}