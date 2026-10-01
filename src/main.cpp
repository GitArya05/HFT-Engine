#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
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
    // 1. Maximize process priority at startup to reduce OS scheduling jitter
    hft::maximize_process_priority();

    constexpr int NUM_GATEWAYS = 4;
    constexpr uint32_t ORDERS_PER_GATEWAY = 25000;
    constexpr uint64_t TOTAL_ORDERS = NUM_GATEWAYS * ORDERS_PER_GATEWAY;
    constexpr size_t QUEUE_CAPACITY = 131072;  // Power of 2 >= TOTAL_ORDERS

    std::cout << "========================================================\n";
    std::cout << "   HFT MATCHING ENGINE: LIVE ORCHESTRATION SIMULATION\n";
    std::cout << "========================================================\n";
    std::cout << " Configuration:\n";
    std::cout << " - Gateway Producer Threads: " << NUM_GATEWAYS << "\n";
    std::cout << " - Orders per Gateway:        " << ORDERS_PER_GATEWAY << "\n";
    std::cout << " - Total Orders to Process:   " << TOTAL_ORDERS << "\n";
    std::cout << " - Inbound Protocol:          NASDAQ OUCH 5.0 (Binary Wire)\n";
    std::cout << " - Core Allocation:           Core 0 (Engine), Cores 1-4 (Gateways)\n";
    std::cout << "--------------------------------------------------------\n\n";

    auto order_queue = std::make_unique<hft::MPSCQueue<InboundOrderMessage, QUEUE_CAPACITY>>();
    auto order_book = std::make_unique<hft::OrderBook>();

    std::atomic<uint64_t> processed_count{0};
    std::atomic<bool> producers_done{false};

    std::cout << "[SIMULATION] Launching engine pipeline...\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. Launch Matching Engine Thread on Core 0 (Time-Critical Priority)
    std::thread engine_thread([&]() {
        hft::configure_current_thread(0);  // Pin to Core 0

        InboundOrderMessage msg;
        while (true) {
            if (order_queue->pop(msg)) {
                order_book->add_order(msg.id, msg.price, msg.qty, msg.is_buy, msg.type);
                uint64_t current_processed = ++processed_count;
                if (current_processed == TOTAL_ORDERS) {
                    break;
                }
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

    // 3. Launch Gateway Producer Threads on Cores 1 through 4
    std::vector<std::thread> gateways;
    gateways.reserve(NUM_GATEWAYS);

    for (int i = 0; i < NUM_GATEWAYS; ++i) {
        gateways.emplace_back([i, ORDERS_PER_GATEWAY, &order_queue]() {
            hft::configure_current_thread(i + 1);  // Pin to Cores 1, 2, 3, 4

            uint64_t base_id = static_cast<uint64_t>(i + 1) * 1000000ULL;
            for (uint32_t j = 0; j < ORDERS_PER_GATEWAY; ++j) {
                uint64_t id = base_id + j;
                uint64_t price = 10000 + (j % 100);
                uint32_t qty = 100 + ((j % 5) * 10);
                bool is_buy = (j % 2 == 0);
                hft::OrderType type = hft::OrderType::LIMIT;

                // Change this:
                while (!order_queue->emplace(id, price, qty, is_buy, type)) {
#if defined(_MSC_VER)
                    _mm_pause();
#endif
                }
            }
        });
    }

    // Wait for all gateway producer threads to complete order generation
    for (auto& gw : gateways) {
        gw.join();
    }
    producers_done.store(true, std::memory_order_release);

    // Wait for engine thread to finish draining the queue and matching orders
    engine_thread.join();

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