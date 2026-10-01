#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <optional>
#include <new>

namespace hft {

// Standard x86 / x64 cacheline size
constexpr std::size_t CACHELINE_SIZE = 64;

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2 for fast bitwise modulo");

public:
    SPSCQueue() : head_(0), tail_(0) {}

    // Non-copyable and non-movable for lock-free thread safety
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Push an item into the queue (Producer Thread only)
    template <typename... Args>
    bool emplace(Args&&... args) {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        const auto current_head = head_.load(std::memory_order_acquire);

        if ((current_tail - current_head) >= Capacity) {
            return false; // Queue full
        }

        buffer_[current_tail & mask_] = T(std::forward<Args>(args)...);
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    // Pop an item from the queue (Consumer Thread only)
    bool pop(T& item) {
        const auto current_head = head_.load(std::memory_order_relaxed);
        const auto current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false; // Queue empty
        }

        item = buffer_[current_head & mask_];
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t size() const {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    [[nodiscard]] constexpr std::size_t capacity() const {
        return Capacity;
    }

private:
    // Align head and tail to distinct cachelines to prevent False Sharing
    alignas(CACHELINE_SIZE) std::atomic<std::size_t> head_{0};
    alignas(CACHELINE_SIZE) std::atomic<std::size_t> tail_{0};

    static constexpr std::size_t mask_ = Capacity - 1;
    alignas(CACHELINE_SIZE) T buffer_[Capacity];
};

} // namespace hft