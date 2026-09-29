#ifndef HFT_ORDER_HPP
#define HFT_ORDER_HPP

#include <cstdint>

namespace hft {

struct Order {
    Order() = default;

    uint64_t id;
    uint64_t price;  // Represented as integer ticks to avoid floating-point math
    uint32_t quantity;
    bool is_buy;

    // Intrusive doubly-linked list pointers for O(1) queue operations
    Order* next{nullptr};
    Order* prev{nullptr};

    Order(uint64_t id_, uint64_t price_, uint32_t qty_, bool buy_)
        : id(id_), price(price_), quantity(qty_), is_buy(buy_) {}
};

}  // namespace hft

#endif  // HFT_ORDER_HPP