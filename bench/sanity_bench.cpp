#include <benchmark/benchmark.h>

// Stage 0 sanity check: proves Google Benchmark is wired up via FetchContent.
// Replaced by real latency/throughput benchmarks starting Stage 2.
static void BM_Sanity(benchmark::State& state) {
    for (auto _ : state) {
        int x = 1 + 1;
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_Sanity);

BENCHMARK_MAIN();
