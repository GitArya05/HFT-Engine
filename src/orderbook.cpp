#include "hft/orderbook.hpp"
#include <algorithm>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace hft {

OrderBook::OrderBook() {
    // Pre-allocate pool (assuming standard array backing for the example)
    order_pool_ = new Order*[100000];
    for (size_t i = 0; i < 100000; ++i) {
        order_pool_[i] = new Order();
    }
}

OrderBook::~OrderBook() {
    for (size_t i = 0; i < 100000; ++i) {
        delete order_pool_[i];
    }
    delete[] order_pool_;
}

Order* OrderBook::allocate_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy,
                                 OrderType type) {
    if (pool_index_ >= 100000) {
        pool_index_ = 0;  // Prevent overflow in simulation bounds
    }
    Order* order = order_pool_[pool_index_++];  // Simplified allocation
    order->id = id;
    order->price = price;
    order->quantity = qty;
    order->is_buy = is_buy;
    order->type = type;
    order->prev = nullptr;
    order->next = nullptr;
    return order;
}

void OrderBook::deallocate_order(Order* order) {
    order_pool_[--pool_index_] = order;  // Simplified deallocation
}

bool OrderBook::has_sufficient_fok_liquidity(uint64_t price, uint32_t qty, bool is_buy) const {
    uint32_t available = 0;
    if (is_buy) {
        for (uint64_t p = best_ask_; p <= price; ++p) {
            Order* current = asks_[p].head;
            while (current) {
                available += current->quantity;
                if (available >= qty)
                    return true;
                current = current->next;
            }
        }
    } else {
        for (uint64_t p = best_bid_; p >= price && p > 0; --p) {
            Order* current = bids_[p].head;
            while (current) {
                available += current->quantity;
                if (available >= qty)
                    return true;
                current = current->next;
            }
        }
    }
    return false;
}

void OrderBook::match_order(Order* incoming) {
    if (incoming->is_buy) {
        while (incoming->quantity > 0 && best_ask_ <= incoming->price) {
            Order* resting = asks_[best_ask_].head;
            if (!resting) {
                best_ask_++;
                continue;
            }

            uint32_t fill_qty = std::min(incoming->quantity, resting->quantity);

            // --- EMIT ITCH 'E' Order Executed Message ---
            if (md_queue_) {
                ItchMessage msg;
                msg.order_executed.message_type = 'E';
                msg.order_executed.timestamp_ns = current_timestamp_ns();
                msg.order_executed.order_ref_number = resting->id;
                msg.order_executed.executed_shares = fill_qty;
                msg.order_executed.match_number = ++match_number_counter_;

                while (!md_queue_->emplace(msg)) {
#if defined(_MSC_VER)
                    _mm_pause();
#endif
                }
            }

            incoming->quantity -= fill_qty;
            resting->quantity -= fill_qty;

            if (resting->quantity == 0) {
                asks_[best_ask_].head = resting->next;
                if (asks_[best_ask_].head)
                    asks_[best_ask_].head->prev = nullptr;
                else
                    asks_[best_ask_].tail = nullptr;
                deallocate_order(resting);
            }
        }
    } else {
        while (incoming->quantity > 0 && best_bid_ >= incoming->price && best_bid_ > 0) {
            Order* resting = bids_[best_bid_].head;
            if (!resting) {
                best_bid_--;
                continue;
            }

            uint32_t fill_qty = std::min(incoming->quantity, resting->quantity);

            // --- EMIT ITCH 'E' Order Executed Message ---
            if (md_queue_) {
                ItchMessage msg;
                msg.order_executed.message_type = 'E';
                msg.order_executed.timestamp_ns = current_timestamp_ns();
                msg.order_executed.order_ref_number = resting->id;
                msg.order_executed.executed_shares = fill_qty;
                msg.order_executed.match_number = ++match_number_counter_;

                while (!md_queue_->emplace(msg)) {
#if defined(_MSC_VER)
                    _mm_pause();
#endif
                }
            }

            incoming->quantity -= fill_qty;
            resting->quantity -= fill_qty;

            if (resting->quantity == 0) {
                bids_[best_bid_].head = resting->next;
                if (bids_[best_bid_].head)
                    bids_[best_bid_].head->prev = nullptr;
                else
                    bids_[best_bid_].tail = nullptr;
                deallocate_order(resting);
            }
        }
    }
}

void OrderBook::add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, OrderType type) {
    if (type == OrderType::FOK && !has_sufficient_fok_liquidity(price, qty, is_buy)) {
        return;  // Kill order
    }

    Order* order = allocate_order(id, price, qty, is_buy, type);
    match_order(order);

    if (order->quantity > 0 && type != OrderType::IOC && type != OrderType::FOK) {
        // --- EMIT ITCH 'A' Add Order Message ---
        if (md_queue_) {
            ItchMessage msg;
            msg.add_order.message_type = 'A';
            msg.add_order.timestamp_ns = current_timestamp_ns();
            msg.add_order.order_ref_number = order->id;
            msg.add_order.buy_sell_indicator = order->is_buy ? 'B' : 'S';
            msg.add_order.shares = order->quantity;
            msg.add_order.price = order->price;

            while (!md_queue_->emplace(msg)) {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }

        PriceLevel& level = is_buy ? bids_[price] : asks_[price];
        if (!level.tail) {
            level.head = level.tail = order;
        } else {
            level.tail->next = order;
            order->prev = level.tail;
            level.tail = order;
        }

        if (is_buy && price > best_bid_)
            best_bid_ = price;
        else if (!is_buy && price < best_ask_)
            best_ask_ = price;
    } else {
        deallocate_order(order);
    }
}

void OrderBook::cancel_order(uint64_t id) {
    // Simplified stub - in real implementation you'd use an unordered_map to find the order $O(1)$
    // Assuming 'target_order' was found:
    /*
    if (md_queue_) {
        ItchMessage msg;
        msg.order_cancel.message_type = 'X';
        msg.order_cancel.timestamp_ns = current_timestamp_ns();
        msg.order_cancel.order_ref_number = target_order->id;
        msg.order_cancel.canceled_shares = target_order->quantity;

        while (!md_queue_->emplace(msg)) {
#if defined(_MSC_VER)
            _mm_pause();
#endif
        }
    }
    */
}

}  // namespace hft