#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include "hft/memory_pool.hpp"
#include "hft/order.hpp"
#include "hft/types.hpp"

namespace hft {

// Unpadded to keep L1/L2 cache dense during order matching
struct PriceLevel {
    uint64_t price{0};
    uint32_t total_volume{0};
    Order* head{nullptr};
    Order* tail{nullptr};

    inline void append(Order* order) {
        order->next = nullptr;
        order->prev = tail;

        if (tail) {
            tail->next = order;
        } else {
            head = order;
        }

        tail = order;
        total_volume += order->quantity;
    }

    inline void remove(Order* order) {
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head = order->next;
        }

        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail = order->prev;
        }

        total_volume -= order->quantity;
        order->next = nullptr;
        order->prev = nullptr;
    }

    inline bool is_empty() const {
        return head == nullptr;
    }
};

class alignas(CACHE_LINE_SIZE) OrderBook {
public:
    static constexpr size_t MAX_PRICE_TICKS = 100000;
    static constexpr size_t MAX_ORDERS = 100000;

    OrderBook();

    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy,
                   OrderType type = OrderType::LIMIT, uint32_t display_qty = 0);

    void cancel_order(Order* order);

    uint64_t get_best_bid() const {
        return best_bid_;
    }
    uint64_t get_best_ask() const {
        return best_ask_;
    }

private:
    alignas(CACHE_LINE_SIZE) OrderPool<MAX_ORDERS> order_pool_;

    alignas(CACHE_LINE_SIZE) std::array<PriceLevel, MAX_PRICE_TICKS> bids_;

    alignas(CACHE_LINE_SIZE) std::array<PriceLevel, MAX_PRICE_TICKS> asks_;

    alignas(CACHE_LINE_SIZE) uint64_t best_bid_{0};
    uint8_t pad_bid_[CACHE_LINE_SIZE - sizeof(uint64_t)]{};

    alignas(CACHE_LINE_SIZE) uint64_t best_ask_{MAX_PRICE_TICKS - 1};
    uint8_t pad_ask_[CACHE_LINE_SIZE - sizeof(uint64_t)]{};

    bool has_sufficient_fok_liquidity(uint64_t price, uint32_t required_qty, bool is_buy) const;
    void match_order(Order* inbound);
};

}  // namespace hft