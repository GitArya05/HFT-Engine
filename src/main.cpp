#include <iostream>
#include "hft/orderbook.hpp"

int main() {
    std::cout << "Initializing High-Frequency Trading Engine..." << std::endl;

    hft::OrderBook book;

    std::cout << "Adding resting orders..." << std::endl;
    // Phase 2 API: add_order(id, price, qty, is_buy)
    book.add_order(1, 100, 10, true);   // Bid: 10 @ 100
    book.add_order(2, 99, 15, true);    // Bid: 15 @ 99
    book.add_order(3, 102, 20, false);  // Ask: 20 @ 102
    book.add_order(4, 103, 5, false);   // Ask: 5 @ 103

    std::cout << "Best Bid: " << book.get_best_bid() << std::endl;
    std::cout << "Best Ask: " << book.get_best_ask() << std::endl;

    std::cout << "Submitting aggressive crossing order..." << std::endl;
    book.add_order(5, 100, 10, false);  // Ask: 10 @ 100 (crosses with Bid 1)

    std::cout << "Engine execution completed successfully." << std::endl;
    return 0;
}