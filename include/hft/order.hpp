#pragma once

#include "hft/types.hpp"

namespace hft {

/// Represents a single resting or incoming order in the matching engine.
struct Order {
    OrderId order_id;
    Price price;
    Quantity quantity;
    bool is_buy;
    Timestamp timestamp;

    Order() = default;

    Order(OrderId id, Price p, Quantity q, bool buy, Timestamp ts)
        : order_id(id), price(p), quantity(q), is_buy(buy), timestamp(ts) {}
};

}  // namespace hft