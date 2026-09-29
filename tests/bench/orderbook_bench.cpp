#include <benchmark/benchmark.h>
#include "hft/orderbook.hpp"

static void BM_OrderBook_AddRestingOrders(benchmark::State& state) {
    for (auto _ : state) {
        hft::OrderBook book;
        for (uint64_t i = 0; i < state.range(0); ++i) {
            // Add bid orders sequentially
            book.add_order(i, 100, 10, true);
        }
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(BM_OrderBook_AddRestingOrders)->Arg(100)->Arg(1000);

static void BM_OrderBook_MatchCycle(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        hft::OrderBook book;
        // Pre-fill order book with resting bids
        for (uint64_t i = 0; i < state.range(0); ++i) {
            book.add_order(i, 100, 10, true);
        }
        state.ResumeTiming();

        // Measure the hot-path: aggressive asks crossing the spread
        for (uint64_t i = 0; i < state.range(0); ++i) {
            book.add_order(state.range(0) + i, 100, 10, false);
        }
    }
}
BENCHMARK(BM_OrderBook_MatchCycle)->Arg(100)->Arg(1000);