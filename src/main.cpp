#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include "hft/mpsc_queue.hpp"
#include "hft/network_server.hpp"
#include "hft/orderbook.hpp"
#include "hft/risk_engine.hpp"
#include "hft/thread_config.hpp"

struct InboundOrderMessage {
    uint64_t id;
    uint64_t price;
    uint32_t qty;
    bool is_buy;
    hft::OrderType type;
    uint64_t timestamp_ns;
};

int main() {
    hft::maximize_process_priority();

    constexpr size_t QUEUE_CAPACITY = 131072;

    std::cout << "========================================================\n";
    std::cout << "   HFT MATCHING ENGINE: LIVE TCP NETWORK GATEWAY\n";
    std::cout << "========================================================\n";
    std::cout << " - Core Allocation: Core 0 (Engine), Core 1 (TCP Server)\n";
    std::cout << " - Listening Port:  8080\n";
    std::cout << "--------------------------------------------------------\n\n";

    auto order_queue = std::make_unique<hft::MPSCQueue<InboundOrderMessage, QUEUE_CAPACITY>>();
    auto order_book = std::make_unique<hft::OrderBook>();
    auto itch_queue = std::make_unique<hft::MarketDataQueue>();

    hft::RiskConfig risk_config{5000, 100000000};
    hft::RiskEngine risk_engine(risk_config);

    order_book->set_market_data_queue(itch_queue.get());
    std::atomic<bool> engine_running{true};

    // Matching Engine Thread (Core 0)
    std::thread engine_thread([&]() {
        hft::configure_current_thread(0);
        InboundOrderMessage msg;
        while (engine_running.load(std::memory_order_relaxed)) {
            if (order_queue->pop(msg)) {
                order_book->add_order(msg.id, msg.price, msg.qty, msg.is_buy, msg.type);
                std::cout << "[ENGINE] Matched/Added Order ID: " << msg.id
                          << " | Price: " << msg.price << " | Qty: " << msg.qty << "\n";
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // Define the callback that fires when a TCP packet arrives
    auto on_network_order = [&](uint64_t id, uint64_t price, uint32_t qty, bool is_buy) -> bool {
        uint64_t rejection_reason = 0;

        // 1. Pass through Pre-Trade Risk
        if (!risk_engine.validate_order(id, price, qty, is_buy, rejection_reason)) {
            std::cout << "[RISK GATEWAY] ORDER REJECTED! ID: " << id
                      << " Reason Code: " << rejection_reason << "\n";
            return false;
        }

        // 2. Inject into Matching Core Queue
        uint64_t send_timestamp =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                      std::chrono::high_resolution_clock::now().time_since_epoch())
                                      .count());

        while (
            !order_queue->emplace(id, price, qty, is_buy, hft::OrderType::LIMIT, send_timestamp)) {
#if defined(_MSC_VER)
            _mm_pause();
#endif
        }
        return true;
    };

    // Spin up TCP Server
    hft::NetworkServer tcp_server(8080, on_network_order);
    if (!tcp_server.initialize()) {
        return -1;
    }

    tcp_server.start_accept_loop();

    std::cout << "\n[SYS] Engine is live. Press ENTER to shutdown...\n\n";
    std::cin.get();

    std::cout << "[SYS] Shutting down...\n";
    tcp_server.stop();
    engine_running.store(false, std::memory_order_relaxed);
    engine_thread.join();

    return 0;
}