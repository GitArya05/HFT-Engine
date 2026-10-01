#pragma once

#include <cstdint>

namespace hft {

struct RiskConfig {
    uint32_t max_order_qty{10000};           // Fat-finger quantity limit
    uint64_t max_order_notional{100000000};  // Max price * qty exposure limit
};

class RiskEngine {
public:
    explicit RiskEngine(const RiskConfig& config) : config_(config) {}

    // Inline, zero-allocation risk validation executed in the hot path (< 10 ns)
    inline bool validate_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy,
                               uint64_t& rejection_reason) const {
        // 1. Zero or negative quantity check
        if (qty == 0) {
            rejection_reason = 1;  // Zero Quantity
            return false;
        }

        // 2. Fat-finger quantity check
        if (qty > config_.max_order_qty) {
            rejection_reason = 2;  // Exceeds Max Order Quantity (Fat-Finger)
            return false;
        }

        // 3. Notional exposure value check
        uint64_t notional = price * static_cast<uint64_t>(qty);
        if (notional > config_.max_order_notional) {
            rejection_reason = 3;  // Exceeds Max Credit / Notional Limit
            return false;
        }

        rejection_reason = 0;
        return true;
    }

private:
    RiskConfig config_;
};

}  // namespace hft