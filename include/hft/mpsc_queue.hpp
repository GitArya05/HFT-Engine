#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

// Hardware Intrinsics for CPU Cache Prefetching
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <xmmintrin.h>
#endif

namespace hft {

template <typename T, std::size_t Capacity>
class MPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    struct alignas(64) Cell {
        std::atomic<std::size_t> sequence;
        T data;
    };

public:
    MPSCQueue() {
        for (std::size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;

    template <typename... Args>
    bool emplace(Args&&... args) {
        Cell* cell = nullptr;
        std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);

        for (;;) {
            cell = &buffer_[pos & mask_];
            std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            // C++20 [[likely]]: We assume the queue usually has space
            if (dif == 0) [[likely]] {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    [[likely]] {
                    break;
                }
            } else if (dif < 0) [[unlikely]] {
                return false;  // Queue is full
            } else {
                pos = enqueue_pos_.load(std::memory_order_relaxed);
            }
        }

        cell->data = T(std::forward<Args>(args)...);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        Cell* cell = &buffer_[pos & mask_];

        // Asynchronously prefetch the NEXT cell into the CPU L1 cache
#if defined(_MSC_VER)
        _mm_prefetch(reinterpret_cast<const char*>(&buffer_[(pos + 1) & mask_]), _MM_HINT_T0);
#else
        __builtin_prefetch(&buffer_[(pos + 1) & mask_], 0, 3);
#endif

        std::size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

        // C++20 [[likely]]: In a busy market, popping usually succeeds
        if (dif == 0) [[likely]] {
            item = std::move(cell->data);
            cell->sequence.store(pos + mask_ + 1, std::memory_order_release);
            dequeue_pos_.store(pos + 1, std::memory_order_relaxed);
            return true;
        }

        return false;
    }

private:
    static constexpr std::size_t mask_ = Capacity - 1;
    alignas(64) Cell buffer_[Capacity];
    alignas(64) std::atomic<std::size_t> enqueue_pos_;
    alignas(64) std::atomic<std::size_t> dequeue_pos_;
};

}  // namespace hft