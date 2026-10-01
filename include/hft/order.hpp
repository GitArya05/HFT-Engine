#pragma once

#include <cstdint>
#include "hft/types.hpp"

namespace hft {

struct Order {
    uint64_t id{0};
    double price{0.0};
    uint32_t quantity{0};
    uint32_t hidden_quantity{0};
    uint32_t peak_quantity{0};
    bool is_buy{true};
    OrderType type{OrderType::LIMIT};
    Order* prev{nullptr};
    Order* next{nullptr};
};

}  // namespace hft