#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "hft/mpsc_queue.hpp"
#include "hft/orderbook.hpp"
#include "hft/ouch_parser.hpp"
#include "hft/thread_utils.hpp"
#include "hft/types.hpp"

// Helper to generate mock NASDAQ OUCH 5.0 binary wire messages
static std::vector<char> create_ouch_enter_order(uint64_t token, char side, uint32_t qty,
                                                 uint32_t price) {
    std::vector<char> buffer(sizeof(hft::OuchEnterOrder), 0);
    auto* msg = reinterpret_cast<hft::OuchEnterOrder*>(buffer.data());

    msg->msg_type = 'O';
    msg->side = side;

    uint64_t be_token = hft::fast_byteswap64(token);
    std::memcpy(msg->order_token, &be_token, 8);

    msg->shares = hft::fast_byteswap32(qty);
    msg->price = hft::fast_byteswap32(price);

    return buffer;
}

int main() {
    constexpr std::size_t NUM_GATEWAYS = 4;
    constexpr std::size_t ORDERS_PER_GATEWAY = 25000;
    constexpr std::size_t TOTAL_ORDERS = NUM_GATEWAYS * ORDERS_PER_GATEWAY;

    // Flush immediately so we know the program successfully started
    std::cout << "========================================================\n"
              << "   HFT MATCHING ENGINE: LIVE ORCHESTRATION SIMULATION   \n"
              << "========================================================\n"
              << " Configuration:\n"
              << "  - Gateway Producer Threads: " << NUM_GATEWAYS << "\n"
              << "  - Orders per Gateway:       " << ORDERS_PER_GATEWAY << "\n"
              << "  - Total Orders to Process:  " << TOTAL_ORDERS << "\n"
              << "  - Inbound Protocol:         NASDAQ OUCH 5.0 (Binary Wire)\n"
              << "  - Core Allocation:          Core 0 (Engine), Cores 1-" << NUM_GATEWAYS
              << " (Gateways)\n"
              << "--------------------------------------------------------\n"
              << std::endl;

    // Allocate massive structures on the heap to prevent stack overflows
    auto pipeline = std::make_unique<hft::MPSCQueue<hft::OrderCommand, 16384>>();
    auto orderbook = std::make_unique<hft::OrderBook>();

    std::atomic<bool> start_flag{false};
    std::atomic<std::size_t> orders_processed{0};

    // -----------------------------------------------------------------
    // 1. CONSUMER THREAD: Matching Engine (Pinned to Core 0)
    // -----------------------------------------------------------------
    auto engine_task = [&]() {
        hft::pin_current_thread_to_core(0);

        // Spin barrier
        while (!start_flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        hft::OrderCommand cmd;
        while (orders_processed.load(std::memory_order_relaxed) < TOTAL_ORDERS) {
            if (pipeline->pop(cmd)) {
                // Accessing orderbook via pointer
                orderbook->add_order(cmd.id, cmd.price, cmd.qty, cmd.is_buy);
                orders_processed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };
    std::thread engine_thread(engine_task);

    // -----------------------------------------------------------------
    // 2. PRODUCER THREADS: Network Gateways (Pinned to Cores 1..4)
    // -----------------------------------------------------------------
    std::vector<std::thread> gateway_threads;
    gateway_threads.reserve(NUM_GATEWAYS);

    for (std::size_t g = 0; g < NUM_GATEWAYS; ++g) {
        auto gateway_task = [&, g]() {
            hft::pin_current_thread_to_core(static_cast<int>(g + 1));

            std::vector<std::vector<char>> ouch_packets;
            ouch_packets.reserve(ORDERS_PER_GATEWAY);

            for (std::size_t i = 0; i < ORDERS_PER_GATEWAY; ++i) {
                uint64_t token = (g * ORDERS_PER_GATEWAY) + i + 1;
                bool is_buy = (i % 2 == 0);
                uint32_t price = is_buy ? (100 + (i % 5)) : (98 + (i % 5));
                uint32_t qty = 10;

                ouch_packets.push_back(
                    create_ouch_enter_order(token, is_buy ? 'B' : 'S', qty, price));
            }

            // Spin barrier
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            // Ingestion loop
            for (const auto& packet : ouch_packets) {
                hft::OrderCommand cmd;
                if (hft::parse_ouch_message(packet.data(), cmd)) {
                    while (!pipeline->emplace(cmd)) {
                        std::this_thread::yield();
                    }
                }
            }
        };
        gateway_threads.emplace_back(gateway_task);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[SIMULATION] Launching engine pipeline...\n" << std::flush;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Release threads
    start_flag.store(true, std::memory_order_release);

    for (auto& gt : gateway_threads) {
        gt.join();
    }
    engine_thread.join();

    auto end_time = std::chrono::high_resolution_clock::now();

    // -----------------------------------------------------------------
    // 3. PERFORMANCE METRICS REPORT
    // -----------------------------------------------------------------
    std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;
    double total_seconds = duration_ms.count() / 1000.0;
    double throughput = static_cast<double>(TOTAL_ORDERS) / total_seconds;
    double latency_ns = (duration_ms.count() * 1'000'000.0) / static_cast<double>(TOTAL_ORDERS);

    std::cout << "\n========================================================\n";
    std::cout << "               SIMULATION RESULTS SUMMARY               \n";
    std::cout << "========================================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << " Total Orders Ingested & Processed: " << orders_processed.load() << "\n";
    std::cout << " Total Execution Time:            " << duration_ms.count() << " ms\n";
    std::cout << " Average End-to-End Latency:      " << latency_ns << " ns / order\n";
    std::cout << " Overall Ingestion Throughput:    " << (throughput / 1'000'000.0)
              << " Million orders/sec\n";
    std::cout << "========================================================\n\n";

    return 0;
}