#ifndef HFT_MEMORY_POOL_HPP
#define HFT_MEMORY_POOL_HPP

#include <array>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <utility>

namespace hft {

/**
 * A fixed-size, contiguous memory pool to avoid heap allocations on the hot path.
 * Designed for single-threaded or thread-local use to avoid lock contention.
 */
template <typename T, std::size_t PoolSize>
class MemoryPool {
private:
    union Node {
        alignas(T) std::byte storage[sizeof(T)];
        Node* next;
    };

    std::array<Node, PoolSize> pool_;
    Node* free_list_{nullptr};

public:
    MemoryPool() {
        if (PoolSize == 0)
            return;

        // Initialize the intrusive free list
        for (std::size_t i = 0; i < PoolSize - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        pool_[PoolSize - 1].next = nullptr;
        free_list_ = &pool_[0];
    }

    // Delete copy and move semantics to prevent accidental copying of the pool
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;

    /**
     * Allocates and constructs an object of type T.
     * Returns nullptr if the pool is exhausted.
     */
    template <typename... Args>
    [[nodiscard]] T* allocate(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))) {
        if (__builtin_expect(!free_list_, 0)) {
            return nullptr;  // Pool exhausted
        }

        Node* node = free_list_;
        free_list_ = node->next;

        // Placement new to construct the object in the pre-allocated storage
        return new (node->storage) T(std::forward<Args>(args)...);
    }

    /**
     * Destroys the object and returns its memory to the pool.
     */
    void deallocate(T* ptr) noexcept {
        if (__builtin_expect(!ptr, 0))
            return;

        ptr->~T();

        // Push back onto the free list
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = free_list_;
        free_list_ = node;
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return PoolSize;
    }
};

}  // namespace hft

#endif  // HFT_MEMORY_POOL_HPP