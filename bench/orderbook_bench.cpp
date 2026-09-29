#include <benchmark/benchmark.h>
#include "hft/orderbook.hpp"

// Benchmark 1: Measure hot-path order insertion and execution latency (Buy + Sell Match cycle)
static void BM_OrderBook_MatchCycle(benchmark::State& state) {
    hft::OrderBook orderbook;
    uint64_t order_id = 1;
    const uint64_t price = 50000;  // Middle tick index

    for (auto _ : state) {
        // Add resting buy order
        orderbook.add_order(order_id++, price, 10, true);

        // Add matching sell order (triggers match_order & deallocates both)
        orderbook.add_order(order_id++, price, 10, false);

        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_OrderBook_MatchCycle);

// Benchmark 2: Measure resting order depth buildup (unmatched limit orders)
static void BM_OrderBook_AddRestingOrders(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        hft::OrderBook orderbook;
        state.ResumeTiming();

        // Populate multiple price levels without matching
        for (uint64_t i = 0; i < 100; ++i) {
            orderbook.add_order(i, 1000 + i, 10, true);
        }

        benchmark::DoNotOptimize(orderbook);
    }
}
BENCHMARK(BM_OrderBook_AddRestingOrders);

BENCHMARK_MAIN();