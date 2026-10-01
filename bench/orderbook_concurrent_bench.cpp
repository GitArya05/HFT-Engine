#include <benchmark/benchmark.h>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include "hft/mpsc_queue.hpp"
#include "hft/thread_utils.hpp"

struct OrderCommand {
    uint64_t id;
    uint32_t price;
    uint32_t qty;
    bool is_buy;
};

static void BM_MPSC_Queue_Ingestion(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();

        // Allocate on the heap to prevent a 1MB Stack Overflow
        auto order_queue = std::make_unique<hft::MPSCQueue<OrderCommand, 16384>>();

        constexpr std::size_t NUM_PRODUCERS = 4;
        constexpr std::size_t ORDERS_PER_PRODUCER = 10000;
        const std::size_t total_orders = NUM_PRODUCERS * ORDERS_PER_PRODUCER;

        std::atomic<bool> start_flag{false};

        // Consumer Thread (Matching Engine)
        std::thread engine_thread([&]() {
            hft::pin_current_thread_to_core(0);  // Lock to CPU Core 0

            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            std::size_t processed = 0;
            OrderCommand cmd;
            while (processed < total_orders) {
                // Use -> instead of . since it's a pointer now
                if (order_queue->pop(cmd)) {
                    benchmark::DoNotOptimize(cmd);
                    processed++;
                } else {
                    std::this_thread::yield();
                }
            }
        });

        // 4 Gateway Producer Threads
        // 4 Gateway Producer Threads - Pin to Cores 1, 2, 3, 4
        std::vector<std::thread> gateway_threads;
        gateway_threads.reserve(NUM_PRODUCERS);
        for (std::size_t p = 0; p < NUM_PRODUCERS; ++p) {
            gateway_threads.emplace_back([&, p]() {
                hft::pin_current_thread_to_core(p + 1);  // Lock to CPU Cores 1 through 4

                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                // ... (rest of gateway loop remains the same)

                for (std::size_t i = 0; i < ORDERS_PER_PRODUCER; ++i) {
                    OrderCommand cmd{static_cast<uint64_t>(p * ORDERS_PER_PRODUCER + i + 1),
                                     static_cast<uint32_t>(100 + (i % 10)), 10, (i % 2 == 0)};
                    while (!order_queue->emplace(cmd)) {
                        std::this_thread::yield();
                    }
                }
            });
        }

        state.ResumeTiming();
        start_flag.store(true, std::memory_order_release);

        for (auto& gt : gateway_threads) {
            gt.join();
        }
        engine_thread.join();
    }
}

BENCHMARK(BM_MPSC_Queue_Ingestion)->Unit(benchmark::kMillisecond)->UseRealTime();

BENCHMARK_MAIN();