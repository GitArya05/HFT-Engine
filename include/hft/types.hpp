#pragma once

#include <cstdint>

namespace hft {

using OrderId = uint64_t;
using Price = uint64_t;
using Quantity = uint32_t;
using Timestamp = uint64_t;

enum class OrderType : uint8_t {
    LIMIT = 0,
    IOC = 1,     // Immediate-Or-Cancel
    FOK = 2,     // Fill-Or-Kill
    ICEBERG = 3  // Iceberg (Hidden Quantity)
};

}  // namespace hft