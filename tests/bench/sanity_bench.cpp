#include <benchmark/benchmark.h>
#include "hft/order.hpp"
#include "hft/orderbook.hpp"

static void BM_OrderBookMatching(benchmark::State& state) {
    for (auto _ : state) {
        hft::OrderBook book;

        // Feed a representative mix of non-crossing and crossing orders
        book.add_order(hft::Order(1, 10050, 100, true, 1000));
        book.add_order(hft::Order(2, 10100, 50, false, 1001));
        book.add_order(hft::Order(3, 10050, 50, true, 1002));
        book.add_order(hft::Order(4, 10000, 100, false, 1003));  // Triggers match
        book.add_order(hft::Order(5, 10200, 200, true, 1004));   // Aggressive match
    }
}
BENCHMARK(BM_OrderBookMatching);

BENCHMARK_MAIN();