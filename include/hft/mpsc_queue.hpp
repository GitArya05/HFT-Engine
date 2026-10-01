#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>
#include "hft/types.hpp"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace hft {

template <typename T, std::size_t Capacity>
class MPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    struct Node {
        std::atomic<std::size_t> sequence{0};
        T data;
    };

public:
    MPSCQueue() : tail_(0), head_(0) {
        for (std::size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    template <typename... Args>
    bool emplace(Args&&... args) {
        Node* node;
        std::size_t pos = tail_.load(std::memory_order_relaxed);

        while (true) {
            node = &buffer_[pos & (Capacity - 1)];
            std::size_t seq = node->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            if (diff == 0) {
                if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
#if defined(_MSC_VER)
                _mm_pause();
#endif
                return false;  // Queue full
            } else {
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        node->data = T(std::forward<Args>(args)...);
        node->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& val) {
        Node* node = &buffer_[head_ & (Capacity - 1)];
        std::size_t seq = node->sequence.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head_ + 1);

        if (diff == 0) {
            val = std::move(node->data);
            node->sequence.store(head_ + Capacity, std::memory_order_release);
            ++head_;
            return true;
        }

#if defined(_MSC_VER)
        _mm_pause();
#endif

        return false;
    }

private:
    // Isolate producer state to prevent false sharing
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_{0};
    uint8_t pad_producer_[CACHE_LINE_SIZE - sizeof(std::atomic<std::size_t>)]{};

    // Isolate consumer state (single consumer, cache-line aligned)
    alignas(CACHE_LINE_SIZE) std::size_t head_{0};
    uint8_t pad_consumer_[CACHE_LINE_SIZE - sizeof(std::size_t)]{};

    // Tightly packed ring buffer
    std::array<Node, Capacity> buffer_;
};

}  // namespace hft