#pragma once

#include <cstddef>
#include <cstdint>

namespace hft {

// Ensures our atomic variables are on separate cache lines to prevent false sharing
constexpr std::size_t CACHE_LINE_SIZE = 64;

enum class OrderType {
    LIMIT,
    GTC,  // Good 'Til Canceled
    IOC,  // Immediate or Cancel
    FOK   // Fill or Kill
};

}  // namespace hft