#include "hft/orderbook.hpp"

namespace hft {

OrderBook::OrderBook() {
    for (size_t i = 0; i < MAX_PRICE_TICKS; ++i) {
        bids_[i].price = i;
        asks_[i].price = i;
    }
}

bool OrderBook::has_sufficient_fok_liquidity(uint64_t price, uint32_t required_qty,
                                             bool is_buy) const {
    uint32_t accumulated_qty = 0;

    if (is_buy) {
        // Sweep asks from best_ask_ up to limit price
        for (uint64_t p = best_ask_; p <= price && p < MAX_PRICE_TICKS; ++p) {
            accumulated_qty += asks_[p].total_volume;
            if (accumulated_qty >= required_qty)
                return true;
        }
    } else {
        // Sweep bids from best_bid_ down to limit price
        for (int64_t p = static_cast<int64_t>(best_bid_);
             p >= static_cast<int64_t>(price) && p >= 0; --p) {
            accumulated_qty += bids_[p].total_volume;
            if (accumulated_qty >= required_qty)
                return true;
        }
    }
    return accumulated_qty >= required_qty;
}

void OrderBook::add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, OrderType type,
                          uint32_t display_qty) {
    if (price >= MAX_PRICE_TICKS || qty == 0)
        return;

    // 1. FOK Pre-flight Check: Reject immediately if liquidity threshold is unmet
    if (type == OrderType::FOK) {
        if (!has_sufficient_fok_liquidity(price, qty, is_buy)) {
            return;  // Kill order without executing any partial matches
        }
    }

    Order* inbound = order_pool_.allocate();
    if (!inbound)
        return;

    inbound->id = id;
    inbound->price = price;
    inbound->is_buy = is_buy;
    inbound->type = type;

    // 2. Setup Iceberg initial display slice vs hidden volume
    if (type == OrderType::ICEBERG && display_qty > 0 && display_qty < qty) {
        inbound->peak_quantity = display_qty;
        inbound->quantity = display_qty;
        inbound->hidden_quantity = qty - display_qty;
    } else {
        inbound->peak_quantity = qty;
        inbound->quantity = qty;
        inbound->hidden_quantity = 0;
    }

    // 3. Execution against book
    match_order(inbound);

    // 4. Post-match Handling
    if (inbound->quantity > 0 || inbound->hidden_quantity > 0) {
        if (type == OrderType::IOC || type == OrderType::FOK) {
            // Cancel remaining unfilled volume immediately
            order_pool_.deallocate(inbound);
        } else {
            // Limit and Iceberg rest on the book
            if (is_buy) {
                bids_[price].append(inbound);
                if (price > best_bid_)
                    best_bid_ = price;
            } else {
                asks_[price].append(inbound);
                if (price < best_ask_)
                    best_ask_ = price;
            }
        }
    } else {
        order_pool_.deallocate(inbound);
    }
}

void OrderBook::match_order(Order* inbound) {
    if (inbound->is_buy) {
        while (inbound->quantity > 0 && best_ask_ <= inbound->price &&
               best_ask_ < MAX_PRICE_TICKS) {
            PriceLevel& level = asks_[best_ask_];
            Order* resting = level.head;

            while (resting && inbound->quantity > 0) {
                Order* next_resting = resting->next;
                uint32_t fill_qty = std::min(inbound->quantity, resting->quantity);

                inbound->quantity -= fill_qty;
                resting->quantity -= fill_qty;
                level.total_volume -= fill_qty;

                // Resting order exhausted
                if (resting->quantity == 0) {
                    if (resting->type == OrderType::ICEBERG && resting->hidden_quantity > 0) {
                        // Replenish visible slice from hidden pool
                        uint32_t reload =
                            std::min(resting->peak_quantity, resting->hidden_quantity);
                        resting->hidden_quantity -= reload;

                        level.remove(resting);
                        resting->quantity = reload;
                        level.append(resting);  // Re-append loses time priority
                    } else {
                        level.remove(resting);
                        order_pool_.deallocate(resting);
                    }
                }

                resting = next_resting;
            }

            if (level.is_empty()) {
                // Advance best_ask_
                while (best_ask_ < MAX_PRICE_TICKS && asks_[best_ask_].is_empty()) {
                    best_ask_++;
                }
            }
        }
    } else {  // Sell order
        while (inbound->quantity > 0 && best_bid_ >= inbound->price &&
               best_bid_ < MAX_PRICE_TICKS) {
            PriceLevel& level = bids_[best_bid_];
            Order* resting = level.head;

            while (resting && inbound->quantity > 0) {
                Order* next_resting = resting->next;
                uint32_t fill_qty = std::min(inbound->quantity, resting->quantity);

                inbound->quantity -= fill_qty;
                resting->quantity -= fill_qty;
                level.total_volume -= fill_qty;

                if (resting->quantity == 0) {
                    if (resting->type == OrderType::ICEBERG && resting->hidden_quantity > 0) {
                        uint32_t reload =
                            std::min(resting->peak_quantity, resting->hidden_quantity);
                        resting->hidden_quantity -= reload;

                        level.remove(resting);
                        resting->quantity = reload;
                        level.append(resting);
                    } else {
                        level.remove(resting);
                        order_pool_.deallocate(resting);
                    }
                }

                resting = next_resting;
            }

            if (level.is_empty()) {
                // Decrease best_bid_
                while (best_bid_ > 0 && bids_[best_bid_].is_empty()) {
                    best_bid_--;
                }
                if (best_bid_ == 0 && bids_[0].is_empty()) {
                    best_bid_ = 0;
                }
            }
        }
    }
}

void OrderBook::cancel_order(Order* order) {
    if (!order)
        return;
    if (order->is_buy) {
        bids_[order->price].remove(order);
    } else {
        asks_[order->price].remove(order);
    }
    order_pool_.deallocate(order);
}

}  // namespace hft