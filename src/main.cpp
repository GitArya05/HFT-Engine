#include "hft/order.hpp"
#include "hft/orderbook.hpp"

#include <chrono>
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== HFT Matching Engine (Phase 1 Baseline) ===\n";

    hft::OrderBook book;

    // 1. Setup mock order flow (using your actual hft::Order struct)
    std::vector<hft::Order> mock_order_flow = {
        {1, 10050, 100, true, 1000},   // Buy  100 @ 100.50
        {2, 10100, 50, false, 1001},   // Sell 50  @ 101.00
        {3, 10050, 50, true, 1002},    // Buy  50  @ 100.50
        {4, 10000, 100, false, 1003},  // Sell 100 @ 100.00 (Crosses with Order 1)
        {5, 10200, 200, true, 1004}    // Aggressive Buy (Wipes out remaining asks)
    };

    std::cout << "Submitting " << mock_order_flow.size() << " orders...\n";

    // 2. Start performance timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // 3. Core Demo Loop
    for (const auto& order : mock_order_flow) {
        book.add_order(order);
    }

    // 4. Stop timer and calculate latency
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    // 5. Output results
    std::cout << "--- Processing Complete ---\n";
    std::cout << "Total Time: " << total_duration.count() << " ns\n";
    std::cout << "Average Latency per Order: " << total_duration.count() / mock_order_flow.size()
              << " ns\n";
    std::cout << "Total Trades Executed: " << book.get_trade_history().size() << "\n";

    return 0;
}