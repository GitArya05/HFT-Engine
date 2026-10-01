#ifndef HFT_ORDER_HPP
#define HFT_ORDER_HPP

#include <cstdint>
#include "hft/types.hpp"

namespace hft {

struct Order {
    OrderId id{0};
    Price price{0};
    Quantity quantity{0};         // Current visible quantity
    Quantity hidden_quantity{0};  // Remaining undisclosed quantity (Iceberg)
    Quantity peak_quantity{0};    // Max visible size per slice (Iceberg)
    bool is_buy{false};
    OrderType type{OrderType::LIMIT};

    Order* next{nullptr};
    Order* prev{nullptr};
};

}  // namespace hft

#endif  // HFT_ORDER_HPP