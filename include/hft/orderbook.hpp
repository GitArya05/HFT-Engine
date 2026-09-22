#pragma once

#include "hft/order.hpp"
#include "hft/types.hpp"

#include <cstdint>
#include <map>
#include <queue>
#include <vector>

namespace hft {

struct Trade {
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
};

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;

    void add_order(Order order);
    bool cancel_order(OrderId order_id, Price price, bool is_buy);

    Price get_best_bid() const;
    Price get_best_ask() const;
    bool has_bids() const {
        return !bids_.empty();
    }
    bool has_asks() const {
        return !asks_.empty();
    }

    const std::vector<Trade>& get_trade_history() const {
        return trade_history_;
    }

private:
    std::map<Price, std::queue<Order>, std::greater<Price>> bids_;
    std::map<Price, std::queue<Order>> asks_;
    std::vector<Trade> trade_history_;

    void match_orders();
};

}  // namespace hft