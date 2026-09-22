#include "hft/orderbook.hpp"

#include <algorithm>

namespace hft {

void OrderBook::add_order(Order order) {
    if (order.quantity == 0) {
        return;
    }

    // Place the order in the appropriate side of the book
    if (order.is_buy) {
        bids_[order.price].push(order);
    } else {
        asks_[order.price].push(order);
    }

    // Trigger matching logic immediately after insertion
    match_orders();
}

void OrderBook::match_orders() {
    while (!bids_.empty() && !asks_.empty()) {
        auto best_bid_it = bids_.begin();
        auto best_ask_it = asks_.begin();

        Price best_bid_price = best_bid_it->first;
        Price best_ask_price = best_ask_it->first;

        // No crossing condition: highest bid is lower than lowest ask
        if (best_bid_price < best_ask_price) {
            break;
        }

        auto& bid_queue = best_bid_it->second;
        auto& ask_queue = best_ask_it->second;

        Order& buy_order = bid_queue.front();
        Order& sell_order = ask_queue.front();

        // Calculate fill quantity
        Quantity traded_qty = std::min(buy_order.quantity, sell_order.quantity);

        // Trade price priority: Resting (maker) order price determines trade price
        Price trade_price =
            (buy_order.timestamp < sell_order.timestamp) ? buy_order.price : sell_order.price;

        Timestamp trade_time = std::max(buy_order.timestamp, sell_order.timestamp);

        // Log trade execution
        trade_history_.push_back(Trade{.buy_order_id = buy_order.order_id,
                                       .sell_order_id = sell_order.order_id,
                                       .price = trade_price,
                                       .quantity = traded_qty,
                                       .timestamp = trade_time});

        // Mutate order quantities
        buy_order.quantity -= traded_qty;
        sell_order.quantity -= traded_qty;

        // Pop fully filled orders and clean empty price levels
        if (buy_order.quantity == 0) {
            bid_queue.pop();
            if (bid_queue.empty()) {
                bids_.erase(best_bid_it);
            }
        }

        if (sell_order.quantity == 0) {
            ask_queue.pop();
            if (ask_queue.empty()) {
                asks_.erase(best_ask_it);
            }
        }
    }
}

bool OrderBook::cancel_order(OrderId order_id, Price price, bool is_buy) {
    auto process_cancellation = [order_id](auto& map, Price p) -> bool {
        auto it = map.find(p);
        if (it == map.end()) {
            return false;
        }

        auto& q = it->second;
        std::queue<Order> updated_q;
        bool found = false;

        // Naive $O(N)$ linear reconstruction of std::queue for order removal
        while (!q.empty()) {
            Order current = q.front();
            q.pop();
            if (current.order_id == order_id && !found) {
                found = true;
            } else {
                updated_q.push(current);
            }
        }

        if (found) {
            it->second = std::move(updated_q);
            if (it->second.empty()) {
                map.erase(it);
            }
            return true;
        }
        return false;
    };

    return is_buy ? process_cancellation(bids_, price) : process_cancellation(asks_, price);
}

Price OrderBook::get_best_bid() const {
    if (bids_.empty())
        return 0;
    return bids_.begin()->first;
}

Price OrderBook::get_best_ask() const {
    if (asks_.empty())
        return 0;
    return asks_.begin()->first;
}

}  // namespace hft