#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include "hft/itch_protocol.hpp"
#include "hft/spsc_queue.hpp"
#include "hft/types.hpp"

namespace hft {

constexpr size_t MD_QUEUE_CAPACITY = 131072;
using MarketDataQueue = SPSCQueue<ItchMessage, MD_QUEUE_CAPACITY>;

// Forward declaration for custom memory pool to avoid circular dependencies
template <typename T, size_t Capacity>
class MemoryPool;

struct Order {
    uint64_t id;
    uint64_t price;
    uint32_t quantity;
    bool is_buy;
    OrderType type;

    Order* prev{nullptr};
    Order* next{nullptr};
};

struct alignas(64) PriceLevel {
    Order* head{nullptr};
    Order* tail{nullptr};
};

class OrderBook {
public:
    OrderBook();
    ~OrderBook();

    // Attach the Outbound Market Data Pipe
    void set_market_data_queue(MarketDataQueue* queue) {
        md_queue_ = queue;
    }

    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, OrderType type);
    void cancel_order(uint64_t id);

private:
    static constexpr size_t MAX_PRICE = 100000;

    std::array<PriceLevel, MAX_PRICE> bids_;
    std::array<PriceLevel, MAX_PRICE> asks_;

    uint64_t best_bid_{0};
    uint64_t best_ask_{MAX_PRICE};

    // Replace with your actual MemoryPool type if named differently
    Order** order_pool_;
    size_t pool_index_{0};
    Order* allocate_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, OrderType type);
    void deallocate_order(Order* order);

    void match_order(Order* incoming);
    bool has_sufficient_fok_liquidity(uint64_t price, uint32_t qty, bool is_buy) const;

    // Market Data State
    MarketDataQueue* md_queue_{nullptr};
    uint64_t match_number_counter_{0};

    // Helper to get nanoseconds since epoch for ITCH timestamps
    inline uint64_t current_timestamp_ns() const {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count());
    }
};

}  // namespace hft