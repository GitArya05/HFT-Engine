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
#include "hft/orderbook.hpp"
#include "hft/thread_config.hpp"

struct InboundOrderMessage {
    uint64_t id;
    uint64_t price;
    uint32_t qty;
    bool is_buy;
    hft::OrderType type;
};

int main() {
    hft::maximize_process_priority();

    constexpr int NUM_GATEWAYS = 4;
    constexpr uint32_t ORDERS_PER_GATEWAY = 25000;
    constexpr uint64_t TOTAL_ORDERS = NUM_GATEWAYS * ORDERS_PER_GATEWAY;
    constexpr size_t QUEUE_CAPACITY = 131072;

    std::cout << "========================================================\n";
    std::cout << "   HFT MATCHING ENGINE: LIVE ORCHESTRATION SIMULATION\n";
    std::cout << "========================================================\n";
    std::cout << " Configuration:\n";
    std::cout << " - Gateway Producer Threads: " << NUM_GATEWAYS << "\n";
    std::cout << " - Orders per Gateway:       " << ORDERS_PER_GATEWAY << "\n";
    std::cout << " - Total Orders to Process:  " << TOTAL_ORDERS << "\n";
    std::cout << " - Core Allocation:          Core 0 (Engine), Cores 1-4 (Gateways), Core 5 "
                 "(Market Data)\n";
    std::cout << "--------------------------------------------------------\n\n";

    // Heap allocation to prevent stack overflow
    auto order_queue = std::make_unique<hft::MPSCQueue<InboundOrderMessage, QUEUE_CAPACITY>>();
    auto order_book = std::make_unique<hft::OrderBook>();
    auto itch_queue = std::make_unique<hft::MarketDataQueue>();

    order_book->set_market_data_queue(itch_queue.get());

    std::atomic<uint64_t> processed_count{0};
    std::atomic<bool> producers_done{false};

    std::cout << "[SIMULATION] Launching engine pipeline...\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Launch Market Data Publisher Thread (Core 5)
    std::thread market_data_thread([&]() {
        hft::configure_current_thread(5);

        hft::ItchMessage md_msg;
        uint64_t messages_published = 0;

        while (true) {
            if (itch_queue->pop(md_msg)) {
                messages_published++;
            } else if (producers_done.load(std::memory_order_acquire) &&
                       processed_count.load(std::memory_order_relaxed) == TOTAL_ORDERS &&
                       itch_queue->empty()) {
                break;
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // 2. Launch Matching Engine Thread (Core 0)
    std::thread engine_thread([&]() {
        hft::configure_current_thread(0);

        InboundOrderMessage msg;
        while (true) {
            if (order_queue->pop(msg)) {
                order_book->add_order(msg.id, msg.price, msg.qty, msg.is_buy, msg.type);
                uint64_t current_processed = ++processed_count;
                if (current_processed == TOTAL_ORDERS)
                    break;
            } else if (producers_done.load(std::memory_order_acquire) &&
                       processed_count.load(std::memory_order_relaxed) == TOTAL_ORDERS) {
                break;
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // 3. Launch Gateway Producer Threads (Cores 1-4)
    std::vector<std::thread> gateways;
    gateways.reserve(NUM_GATEWAYS);

    for (int i = 0; i < NUM_GATEWAYS; ++i) {
        gateways.emplace_back([i, ORDERS_PER_GATEWAY, &order_queue]() {
            hft::configure_current_thread(i + 1);

            uint64_t base_id = static_cast<uint64_t>(i + 1) * 1000000ULL;
            for (uint32_t j = 0; j < ORDERS_PER_GATEWAY; ++j) {
                uint64_t id = base_id + j;
                uint64_t price = 50000 + (j % 100);
                uint32_t qty = 100 + ((j % 5) * 10);
                bool is_buy = (j % 2 == 0);

                while (!order_queue->emplace(id, price, qty, is_buy, hft::OrderType::LIMIT)) {
#if defined(_MSC_VER)
                    _mm_pause();
#endif
                }
            }
        });
    }

    // Await completion
    for (auto& gw : gateways) gw.join();
    producers_done.store(true, std::memory_order_release);
    engine_thread.join();
    market_data_thread.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    double total_sec = duration_ms / 1000.0;
    double throughput = static_cast<double>(TOTAL_ORDERS) / total_sec / 1'000'000.0;
    double avg_latency_ns = (duration_ms * 1'000'000.0) / static_cast<double>(TOTAL_ORDERS);

    std::cout << "========================================================\n";
    std::cout << "                SIMULATION RESULTS SUMMARY\n";
    std::cout << "========================================================\n";
    std::cout << " Total Orders Ingested & Processed: " << TOTAL_ORDERS << "\n";
    std::cout << " Total Execution Time:            " << duration_ms << " ms\n";
    std::cout << " Average End-to-End Latency:      " << avg_latency_ns << " ns / order\n";
    std::cout << " Overall Ingestion Throughput:    " << throughput << " Million orders/sec\n";
    std::cout << "========================================================\n";

    return 0;
}