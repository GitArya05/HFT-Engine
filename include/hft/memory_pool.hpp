#pragma once

#include <cassert>
#include <cstdint>
#include <vector>
#include "hft/order.hpp"

namespace hft {

template <size_t Capacity = 1024 * 64>  // Default pool size: 64k orders
class OrderPool {
public:
    OrderPool() {
        // Pre-allocate contiguous memory buffer for orders
        pool_.resize(Capacity);
        // Initialize free list pointers
        free_indices_.resize(Capacity);
        for (size_t i = 0; i < Capacity; ++i) {
            free_indices_[i] = Capacity - 1 - i;
        }
        free_count_ = Capacity;
    }

    ~OrderPool() = default;

    // Non-copyable, non-movable for safety
    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    // Allocate an order slot in O(1) time
    template <typename... Args>
    Order* allocate(Args&&... args) {
        if (free_count_ == 0) {
            return nullptr;  // Pool exhausted
        }

        size_t idx = free_indices_[--free_count_];
        Order* ptr = &pool_[idx];

        // Construct in-place using placement new
        new (ptr) Order(std::forward<Args>(args)...);
        return ptr;
    }

    // Deallocate / recycle an order slot in O(1) time
    void deallocate(Order* order) {
        if (!order)
            return;

        // Calculate index via pointer arithmetic
        size_t idx = order - pool_.data();
        assert(idx < Capacity);

        order->~Order();  // Explicit destructor call
        free_indices_[free_count++] = idx;
    }

    void reset() {
        free_count_ = Capacity;
        for (size_t i = 0; i < Capacity; ++i) {
            free_indices_[i] = Capacity - 1 - i;
        }
    }

    size_t available() const {
        return free_count_;
    }
    size_t capacity() const {
        return Capacity;
    }

private:
    std::vector<Order> pool_;
    std::vector<size_t> free_indices_;
    size_t free_count_;
};

}  // namespace hft