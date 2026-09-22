#pragma once

#include <cstdint>

namespace hft {

// Type aliases for strong typing and easier future refactoring.
// Using fixed-width integers ensures cross-platform consistency.
using OrderId = uint64_t;

// Fixed-point representation (e.g., pennies or fractions of a penny)
// to prevent floating-point rounding errors on the hot path.
using Price = uint64_t;

using Quantity = uint32_t;

// Represents time in nanoseconds since epoch for precise time-priority matching.
using Timestamp = uint64_t;

}  // namespace hft