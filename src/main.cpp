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
#include "hft/orderbook.hpp"
#include "hft/risk_engine.hpp"
#include "hft/thread_config.hpp"

namespace hft {

class LatencyTracker {
public:
    explicit LatencyTracker(size_t capacity) {
        samples_.reserve(capacity);
    }

    inline void record(uint64_t latency_ns) {
        samples_.push_back(latency_ns);
    }

    void print_report() const {
        if (samples_.empty()) {
            std::cout << "[LATENCY] No samples recorded.\n";
            return;
        }

        std::vector<uint64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        size_t total = sorted.size();
        uint64_t min_val = sorted.front();
        uint64_t max_val = sorted.back();

        uint64_t p50 = sorted[static_cast<size_t>(total * 0.50)];
        uint64_t p90 = sorted[static_cast<size_t>(total * 0.90)];
        uint64_t p99 = sorted[static_cast<size_t>(total * 0.99)];
        uint64_t p999 = sorted[static_cast<size_t>(total * 0.999)];

        uint64_t sum = 0;
        for (auto val : sorted) sum += val;
        double mean = static_cast<double>(sum) / total;

        std::cout << "========================================================\n";
        std::cout << "             TAIL LATENCY PERCENTILES (NS)\n";
        std::cout << "========================================================\n";
        std::cout << " Sample Count: " << total << "\n";
        std::cout << " Min Latency:  " << min_val << " ns\n";
        std::cout << " P50 (Median): " << p50 << " ns\n";
        std::cout << " P90 Latency:  " << p90 << " ns\n";
        std::cout << " P99 Latency:  " << p99 << " ns\n";
        std::cout << " P99.9 Latency:" << p999 << " ns\n";
        std::cout << " Max Latency:  " << max_val << " ns\n";
        std::cout << " Mean Latency: " << mean << " ns\n";
        std::cout << "========================================================\n";
    }

private:
    std::vector<uint64_t> samples_;
};

}  // namespace hft

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

    constexpr int NUM_GATEWAYS = 4;
    constexpr uint32_t ORDERS_PER_GATEWAY = 25000;
    constexpr uint64_t TOTAL_ORDERS = NUM_GATEWAYS * ORDERS_PER_GATEWAY;
    constexpr size_t QUEUE_CAPACITY = 131072;

    std::cout << "========================================================\n";
    std::cout << "   HFT MATCHING ENGINE: RISK GATEWAY & PROFILING SIM\n";
    std::cout << "========================================================\n";
    std::cout << " Configuration:\n";
    std::cout << " - Gateway Producer Threads: " << NUM_GATEWAYS << "\n";
    std::cout << " - Orders per Gateway:       " << ORDERS_PER_GATEWAY << "\n";
    std::cout << " - Total Orders Generated:   " << TOTAL_ORDERS << "\n";
    std::cout << " - Pre-Trade Risk Filter:    Active (Fat-Finger & Notional Limits)\n";
    std::cout << " - Core Allocation:          Core 0 (Engine), Cores 1-4 (Gateways), Core 5 "
                 "(Market Data)\n";
    std::cout << "--------------------------------------------------------\n\n";

    auto order_queue = std::make_unique<hft::MPSCQueue<InboundOrderMessage, QUEUE_CAPACITY>>();
    auto order_book = std::make_unique<hft::OrderBook>();
    auto itch_queue = std::make_unique<hft::MarketDataQueue>();
    auto latency_tracker = std::make_unique<hft::LatencyTracker>(TOTAL_ORDERS);

    hft::RiskConfig risk_config{5000, 100000000};  // Max qty 5000, max notional 100M
    hft::RiskEngine risk_engine(risk_config);

    order_book->set_market_data_queue(itch_queue.get());

    std::atomic<uint64_t> processed_count{0};
    std::atomic<uint64_t> rejected_risk_count{0};
    std::atomic<bool> producers_done{false};

    std::cout << "[SIMULATION] Launching pipeline with pre-trade risk screening...\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Market Data Publisher Thread (Core 5)
    std::thread market_data_thread([&]() {
        hft::configure_current_thread(5);
        hft::ItchMessage md_msg;
        while (true) {
            if (itch_queue->pop(md_msg)) {
                // Handled
            } else if (producers_done.load(std::memory_order_acquire) &&
                       processed_count.load(std::memory_order_relaxed) +
                               rejected_risk_count.load(std::memory_order_relaxed) >=
                           TOTAL_ORDERS &&
                       itch_queue->empty()) {
                break;
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // Matching Engine Thread (Core 0)
    std::thread engine_thread([&]() {
        hft::configure_current_thread(0);
        InboundOrderMessage msg;
        while (true) {
            if (order_queue->pop(msg)) {
                uint64_t receive_timestamp = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch())
                        .count());

                order_book->add_order(msg.id, msg.price, msg.qty, msg.is_buy, msg.type);

                uint64_t latency = receive_timestamp - msg.timestamp_ns;
                latency_tracker->record(latency);

                processed_count.fetch_add(1, std::memory_order_relaxed);
            } else if (producers_done.load(std::memory_order_acquire) &&
                       processed_count.load(std::memory_order_relaxed) ==
                           (TOTAL_ORDERS - rejected_risk_count.load(std::memory_order_relaxed))) {
                break;
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // Gateway Producer Threads (Cores 1-4) with Pre-Trade Risk Filtering
    std::vector<std::thread> gateways;
    gateways.reserve(NUM_GATEWAYS);

    for (int i = 0; i < NUM_GATEWAYS; ++i) {
        gateways.emplace_back(
            [i, ORDERS_PER_GATEWAY, &order_queue, &risk_engine, &rejected_risk_count]() {
                hft::configure_current_thread(i + 1);

                uint64_t base_id = static_cast<uint64_t>(i + 1) * 1000000ULL;
                for (uint32_t j = 0; j < ORDERS_PER_GATEWAY; ++j) {
                    uint64_t id = base_id + j;
                    uint64_t price = 50000 + (j % 100);
                    uint32_t qty = 100 + ((j % 5) * 10);
                    bool is_buy = (j % 2 == 0);

                    // Inject a synthetic risk violation test case every 10,000 orders to verify the
                    // risk gateway
                    if (j == 500) {
                        qty = 99999;  // Fat-finger violation (> 5000 max qty)
                    }

                    uint64_t rejection_reason = 0;
                    if (!risk_engine.validate_order(id, price, qty, is_buy, rejection_reason)) {
                        rejected_risk_count.fetch_add(1, std::memory_order_relaxed);
                        continue;  // Drop order before it ever hits the lock-free queue or matching
                                   // core
                    }

                    uint64_t send_timestamp = static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::high_resolution_clock::now().time_since_epoch())
                            .count());

                    while (!order_queue->emplace(id, price, qty, is_buy, hft::OrderType::LIMIT,
                                                 send_timestamp)) {
#if defined(_MSC_VER)
                        _mm_pause();
#endif
                    }
                }
            });
    }

    for (auto& gw : gateways) gw.join();
    producers_done.store(true, std::memory_order_release);
    engine_thread.join();
    market_data_thread.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    uint64_t total_processed = processed_count.load();
    uint64_t total_rejected = rejected_risk_count.load();
    double total_sec = duration_ms / 1000.0;
    double throughput = static_cast<double>(total_processed) / total_sec / 1'000'000.0;

    std::cout << "========================================================\n";
    std::cout << "                SIMULATION RESULTS SUMMARY\n";
    std::cout << "========================================================\n";
    std::cout << " Total Orders Evaluated by Risk:  " << (total_processed + total_rejected) << "\n";
    std::cout << " Orders Passed to Matching Engine:" << total_processed << "\n";
    std::cout << " Orders Blocked by Risk Gateway:  " << total_rejected
              << " (Fat-finger caught!)\n";
    std::cout << " Total Execution Time:            " << duration_ms << " ms\n";
    std::cout << " Effective Ingestion Throughput:  " << throughput << " Million orders/sec\n";
    std::cout << "========================================================\n\n";

    latency_tracker->print_report();

    return 0;
}