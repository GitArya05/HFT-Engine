#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>
#include "hft/types.hpp"

namespace hft {

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SPSCQueue() : tail_(0), head_(0) {}

    template <typename... Args>
    bool emplace(Args&&... args) {
        const std::size_t current_tail = tail_.load(std::memory_order_relaxed);
        const std::size_t current_head = head_.load(std::memory_order_acquire);

        if (current_tail - current_head >= Capacity) {
            return false;  // Queue full
        }

        buffer_[current_tail & (Capacity - 1)] = T(std::forward<Args>(args)...);
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& val) {
        const std::size_t current_head = head_.load(std::memory_order_relaxed);
        const std::size_t current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false;  // Queue empty
        }

        val = std::move(buffer_[current_head & (Capacity - 1)]);
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_{0};
    uint8_t pad_producer_[CACHE_LINE_SIZE - sizeof(std::atomic<std::size_t>)]{};

    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head_{0};
    uint8_t pad_consumer_[CACHE_LINE_SIZE - sizeof(std::atomic<std::size_t>)]{};

    std::array<T, Capacity> buffer_;
};

}  // namespace hft