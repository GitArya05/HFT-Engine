# HFT Matching Engine

A high-performance High-Frequency Trading (HFT) Matching Engine built in C++20. This project intentionally avoids real trading product overhead and focuses purely on deep systems-engineering challenges: zero heap allocation on the hot path, CPU cache locality, and lock-free concurrency.

## 🚀 Architecture & Phases

### Phase 1: Correctness-First Baseline (✅ Completed)
- Established a naive order book baseline using `std::map` and `std::queue` to verify correctness.
- Implemented price-time priority matching logic using fixed-point integer types (`uint64_t`) to prevent floating-point precision errors.
- Comprehensive unit test coverage using GoogleTest.
- Baseline performance metrics captured via Google Benchmark (Latency: ~39,720 ns per order).

### Phase 2: High-Performance Optimizations (🚧 In Progress)
- **Zero Heap Allocation**: Replaced dynamic heap allocations (`new`/`malloc`) with a custom fixed-capacity `MemoryPool` (Arena Allocator) to achieve $O(1)$ order allocations on the hot path.
- **CPU Cache Locality**: (Upcoming) Transitioning from node-based dynamic containers to flat arrays and ring buffers to maximize L1/L2 cache hits.
- **Lock-Free Concurrency**: (Upcoming) Implementing SPSC/MPSC queues to prepare the engine for multi-threaded order ingestion without lock contention.

## 🛠️ Tech Stack
- **Language**: C++20
- **Build System**: CMake, MSVC
- **Testing**: GoogleTest (GTest)
- **Benchmarking**: Google Benchmark

## ⚙️ Build & Run Instructions (Windows / MSVC)

1. **Configure the build**:
   ```powershell
   cmake -DCMAKE_BUILD_TYPE=Release -B build

2. **Compile the project**:
   ```powershell
   cmake --build build --config Release

3. **Run unit tests**:
   ```powershell
   .\build\tests\Release\hft_tests.exe

4. **Run Benchmarks**:
   ```powershell
   .\build\bench\Release\hft_bench.exe

****CRAZY ARYA****
   
   
