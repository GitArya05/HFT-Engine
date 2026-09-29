#include "hft/orderbook.hpp"
#include <algorithm>

namespace hft {

OrderBook::OrderBook() {
    // Initialize inside market to extremes
    best_bid_ = 0;
    best_ask_ = MAX_PRICE_TICKS - 1;
}

void OrderBook::add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy) {
    // 1. O(1) Allocation from MemoryPool (Zero Heap Allocation)
    Order* order = order_pool_.allocate(id, price, qty, is_buy);
    if (!order) {
        return;  // Pool exhausted (in production, handle failure gracefully)
    }

    // 2. Attempt to cross the spread and match aggressively
    match_order(order);

    // 3. If the order isn't fully filled, add remaining qty to the resting book
    if (order->quantity > 0) {
        if (is_buy) {
            bids_[price].append(order);
            // Update best bid if necessary
            if (price > best_bid_) {
                best_bid_ = price;
            }
        } else {
            asks_[price].append(order);
            // Update best ask if necessary
            if (price < best_ask_) {
                best_ask_ = price;
            }
        }
    } else {
        // Order was completely filled immediately; recycle the memory slot
        order_pool_.deallocate(order);
    }
}

void OrderBook::match_order(Order* inbound) {
    if (inbound->is_buy) {
        // Buy order: Match against resting Asks (lowest price first)
        while (inbound->quantity > 0 && best_ask_ <= inbound->price &&
               best_ask_ < MAX_PRICE_TICKS) {
            PriceLevel& level = asks_[best_ask_];
            Order* resting = level.head;

            while (resting != nullptr && inbound->quantity > 0) {
                uint32_t fill_qty = std::min(inbound->quantity, resting->quantity);

                // Execute trade (In a real system, generate trade events here)
                inbound->quantity -= fill_qty;
                resting->quantity -= fill_qty;
                level.total_volume -= fill_qty;  // Deduct volume manually during match

                Order* next_resting = resting->next;  // Cache next pointer before removal

                // If resting order is fully filled, remove from list and deallocate
                if (resting->quantity == 0) {
                    level.remove(resting);
                    order_pool_.deallocate(resting);
                }

                resting = next_resting;
            }

            // If price level is completely drained, advance best_ask_ upward
            if (level.is_empty()) {
                best_ask_++;
            }
        }
    } else {
        // Sell order: Match against resting Bids (highest price first)
        while (inbound->quantity > 0 && best_bid_ >= inbound->price && best_bid_ > 0) {
            PriceLevel& level = bids_[best_bid_];
            Order* resting = level.head;

            while (resting != nullptr && inbound->quantity > 0) {
                uint32_t fill_qty = std::min(inbound->quantity, resting->quantity);

                // Execute trade
                inbound->quantity -= fill_qty;
                resting->quantity -= fill_qty;
                level.total_volume -= fill_qty;

                Order* next_resting = resting->next;

                // If resting order is fully filled, remove from list and deallocate
                if (resting->quantity == 0) {
                    level.remove(resting);
                    order_pool_.deallocate(resting);
                }

                resting = next_resting;
            }

            // If price level is completely drained, advance best_bid_ downward
            if (level.is_empty()) {
                best_bid_--;
            }
        }
    }
}

void OrderBook::cancel_order(Order* order) {
    if (!order)
        return;

    // Determine side and access the specific price level
    if (order->is_buy) {
        bids_[order->price].remove(order);

        // Downward walk to find the new best_bid_ if we drained the top level
        if (order->price == best_bid_ && bids_[best_bid_].is_empty()) {
            while (best_bid_ > 0 && bids_[best_bid_].is_empty()) {
                best_bid_--;
            }
        }
    } else {
        asks_[order->price].remove(order);

        // Upward walk to find the new best_ask_ if we drained the top level
        if (order->price == best_ask_ && asks_[best_ask_].is_empty()) {
            while (best_ask_ < MAX_PRICE_TICKS - 1 && asks_[best_ask_].is_empty()) {
                best_ask_++;
            }
        }
    }

    // Recycle the memory slot
    order_pool_.deallocate(order);
}

}  // namespace hft