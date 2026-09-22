# CLAUDE.md — Zero-Allocation, Ultra-Low-Latency Matching Engine

This file is the persistent context for working on this project with
Claude. Read it at the start of every session.

## What this project is

A matching engine for one symbol ("SYM1"). It receives buy/sell limit
orders, matches them by price-time priority, produces trades, and supports
cancels — all in memory. **It is not a real trading product.** It exists to
demonstrate systems-engineering skills (memory management, CPU cache
behavior, lock-free concurrency, networking, benchmarking) for backend /
systems / performance-engineering interviews. The "not real" part is the
trading domain; the systems engineering is the real, gradeable part.

## Who is building this

A final-year engineering student learning C++ systems programming, using
Claude as a mentor/pair-programmer. **The student writes the core logic
themselves.** Claude explains concepts, designs interfaces, writes
boilerplate, and reviews critically — see "How to work with me" below.

## Architecture (3 layers)

```
 ┌─────────────────────┐        ┌──────────────────────────┐        ┌───────────────────────────┐
 │ LAYER 3: DASHBOARD   │ HTTP/  │ LAYER 2: GATEWAY          │ TCP,   │ LAYER 1: C++ ENGINE        │
 │ React + Vite         │◀──────▶│ FastAPI (Python)          │◀──────▶│ (Stages 0-7)               │
 │                      │  WS    │                           │ binary │                            │
 │ - order book depth   │        │ - REST: POST /orders,     │ proto  │ IOCP network thread        │
 │ - trade tape          │        │   DELETE /orders/{id}    │        │        <-> SPSC rings       │
 │ - order entry form   │        │ - WS: /ws/marketdata      │        │ matching thread (pinned)   │
 │ - live p50/p99/p99.9  │        │   (book + trade stream)  │        │ pool-allocated flat book   │
 │   /max latency charts│        │ - ONE persistent TCP conn │        │ snapshot thread            │
 │                      │        │   to the engine, fans out │        │                            │
 │                      │        │   to N browsers           │        │                            │
 └─────────────────────┘        └──────────────────────────┘        └────────────────────────────┘
                                                                              ▲
                                                                              │ raw TCP, binary protocol
                                                                     ┌────────┴────────┐
                                                                     │ tools/order_gen  │  <- latency numbers
                                                                     │ client/ (Python) │     come from HERE,
                                                                     └──────────────────┘     never via the gateway
```

**Layer 1 (C++ engine) is the core of the project** and is built, tested,
and benchmarked to completion before Layers 2-3 are touched (Stages 0-7,
then 8-10). The gateway is a translator (binary protocol <-> JSON/WebSocket),
never a decision-maker — no matching logic lives outside the engine. Latency
is always measured by talking to the engine directly (`client/` or
`tools/order_gen`), never through the gateway or browser, or the numbers
measure Python/JS overhead instead of the engine.

## Platform & toolchain (Windows-native — see rationale below)

- **OS:** Windows 11. Development and Stages 0-6 run entirely on Windows.
- **Compiler:** MSVC (Visual Studio Build Tools, `cl.exe`). Verified on this
  machine: VS installationVersion `18.7.11911.148`, `cl` `19.51.36248`, full
  C++20 support.
- **Build:** CMake + Ninja (bundled with VS Build Tools). **Important
  quirk:** this CMake's `-G "Visual Studio 17 2022"` generator does **not**
  recognize this VS install (tested and confirmed broken). Use the **Ninja**
  generator instead, with the MSVC environment sourced via `vcvars64.bat`
  first (`scripts/build.ps1` does this, or use VS Code + CMake Tools, which
  detects the kit automatically).
- **Warnings:** `/W4 /permissive-` on MSVC (equivalent to `-Wall -Wextra
  -Wpedantic` on GCC/Clang, used in Linux CI).
- **Sanitizers:** MSVC `/fsanitize=address` for Debug. No MSVC UBSan and no
  reliable Windows TSan — see "Known platform gaps" below.
- **Optimization:** MSVC has no `/O3`; Release uses the default MSVC
  `/O2` (its maximum general optimization level).
- **Tests:** GoogleTest. **Benchmarks:** Google Benchmark + a custom
  latency-histogram harness. Both via CMake FetchContent. No Boost, no
  third-party allocators or queues — those are built from scratch.
- **Frontend (Stage 9):** React + Vite (not Next.js — it's a pure client of
  the FastAPI backend, so Next.js's SSR/API-route machinery would sit
  unused).

### Why Windows, not Linux (the original plan)

The original plan targeted Ubuntu (POSIX sockets + `epoll`, `fork`-based
snapshots, ThreadSanitizer, `perf`/flamegraphs). The student explicitly
chose to redesign for Windows rather than use WSL2/EC2 for development.
This is a genuine re-architecture of Layer 1, not a search-and-replace:

| Original (Linux) | Windows replacement |
|---|---|
| `epoll` | **IOCP** (chosen over WSAPoll — bigger lift, but the real high-performance Windows async I/O model, not just a compatibility shim) |
| `pthread_setaffinity_np` | `SetThreadAffinityMask` |
| `mmap` (snapshot file) | `CreateFileMapping` + `MapViewOfFile` |
| `fork()`-based COW snapshot | No `fork()` on Windows — replaced with a **double-buffer / seqlock** technique read by a background thread (arguably a better lesson: a data-structure technique instead of an OS trick) |
| ThreadSanitizer | See "Known platform gaps" |
| `perf stat` + flamegraphs | See "Known platform gaps" |

### Known platform gaps and mitigations

1. **No reliable ThreadSanitizer on Windows.** Mitigation: a GitHub Actions
   `ubuntu-latest` CI job builds the concurrency code (SPSC ring buffer,
   matching/network threads) with GCC/Clang `-fsanitize=thread` and runs the
   concurrency tests there. Added at **Stage 4** when that code exists (see
   the comment at the bottom of `.github/workflows/ci.yml`).
2. **No `perf`/flamegraphs on Windows.** Mitigation: final Stage 7 numbers
   and flamegraphs come from **Intel VTune or Windows Performance
   Analyzer**, run on a **Windows Server EC2 compute-optimized instance**
   (described honestly as a VM, not bare metal — consistent with the
   original "no containers, honest VM" rule).

Both gaps and their mitigations are documented honestly in the final README
(Stage 7) rather than silently dropped.

## Design decisions (already made — follow them)

- Prices are **integer ticks** (`int64_t`, e.g. 50.10 -> 5010). Never
  `double`. Convert only at I/O edges.
- Type aliases (`Price`, `Qty`, `OrderId`, `Side`) live in one header,
  `include/hft/types.hpp` (added Stage 1).
- Every order-book implementation shares the same interface so they can be
  tested and benchmarked against each other interchangeably.
- The Stage 1 naive book (`std::map`-based) is kept forever as the
  **reference oracle**: a randomized differential test (fixed seed, millions
  of orders/cancels) must produce identical trades from every later
  implementation.
- Every optimized version must pass the differential test. **Never silently
  skip correctness for speed.**
- Assertions in tests must not depend on `NDEBUG`/Release builds — use an
  always-on assert macro, not bare `assert()`.
- Zero heap allocation on the hot path (custom pool allocator, preallocated
  at startup).
- Cache locality: flat contiguous arrays instead of `std::map`, once we get
  to Stage 3.
- Honest zero-copy: `Order` is a trivially-copyable struct, so `std::move`
  gives it no benefit. Real zero-copy is constructing in place in the ring
  buffer and passing pool indices/pointers, not the struct itself.
- Lock-free SPSC ring buffer: `std::atomic`, acquire/release ordering,
  head/tail padded to separate cache lines (no false sharing).
- Latency measured as **p50 / p99 / p99.9 / max**, never just averages,
  recorded into preallocated arrays. Threads pinned to cores.

## Folder structure

```
HFT/
├── CMakeLists.txt / CMakePresets.json   MSVC + Ninja build, FetchContent for GTest/Benchmark
├── CLAUDE.md / README.md
├── .clang-format / .gitignore
├── .github/workflows/ci.yml             Windows build+test now; Linux TSan job added Stage 4
├── include/hft/                         PUBLIC headers, namespace hft:: (empty until Stage 1)
├── src/                                 non-template code, server entry point
├── tests/                               GoogleTest: unit tests + the differential test
├── bench/                               Google Benchmark + latency-histogram harness
├── tools/                               order_gen (Stage 2), book_cli (Stage 1)
├── client/                              Python low-level test client + load generator (Stage 5)
├── scripts/                             build.ps1 (MSVC+Ninja wrapper) and future helpers
├── docs/                                design notes + benchmark results per stage
├── gateway/  (reserved, Stage 8)        FastAPI service — off the hot path
├── web/      (reserved, Stage 9)        React + Vite dashboard
└── docker/   (reserved, Stage 10)       docker-compose demo bundle (gateway+web only)
```

Each reserved/empty folder has its own `README.md` explaining what lands
there and when.

## Stage plan

**Stages 0-7 build the C++ engine and are completed in full before Stages
8-10 (full-stack layer) begin.**

| Stage | Adds |
|---|---|
| 0 | Repo, folders, CMake+presets, GoogleTest/Benchmark via FetchContent, sanitizer build, `.gitignore`, `.clang-format`, `CLAUDE.md`, README skeleton |
| 1 | Naive order book: `std::map<Price, std::deque<Order>>` per side + `unordered_map` index for cancels. Input validation. Read-only query API (`bestBid`, `bestAsk`, `volumeAt`). Full GoogleTest suite. Terminal CLI. |
| 2 | Deterministic random order-stream generator (fixed seed), differential test harness, latency-histogram benchmark harness. Baseline numbers for the naive book. |
| 3 | Cache-friendly book: flat price-indexed arrays, intrusive FIFO lists in a preallocated pool, free-list allocation, no `new`/`delete` on the hot path. Must pass the differential test. Benchmark vs. baseline. |
| 4 | Concurrency: SPSC ring buffer (atomics, acquire/release, cache-line padding), threads for input -> matching -> output, core pinning via `SetThreadAffinityMask`. Unit tests. **Linux CI TSan job added here.** Benchmark. |
| 5 | Networking: Winsock + **IOCP**, fixed-layout binary protocol with explicit framing, per-connection receive buffers handling partial/coalesced messages. Python mock trader + load generator (`client/`). |
| 6 | Snapshotting & recovery: periodic snapshot off the hot path via double-buffer/seqlock + `CreateFileMapping`/`MapViewOfFile`, restore on startup, test restored book == original. |
| 7 | Benchmarking & polish: VTune/WPA (cache misses, cycles) on a Windows Server EC2 instance, flamegraphs, p50/p99/p99.9/max tables per stage, README with architecture diagram and results. |
| 8 | FastAPI gateway: persistent TCP client to the engine, REST + WebSocket, translates binary protocol <-> JSON. |
| 9 | React + Vite dashboard: order book depth, trade tape, order entry, live latency charts. |
| 10 | Full-stack polish: `docker-compose.yml` for gateway+web demo (engine's own Stage 7 benchmark methodology is unaffected by this). |

## How to work with me (rules Claude follows)

- Go **one stage at a time**, and within a stage, **one small step at a
  time**. After each step, **stop and wait** for the student.
- For each step: (1) explain the concept and *why* in plain language, (2)
  show the design (structs, function signatures, data layout), (3) let the
  student write the core logic — skeleton with TODOs and hints, not the
  finished answer, (4) review their code critically: bugs, undefined
  behavior, performance, style. Be honest about mistakes.
- Claude may write boilerplate (CMake, test scaffolding, scripts, CLI
  parsing) — explained briefly, not treated as a learning exercise.
- If the student is stuck after a real attempt and asks, give progressively
  bigger hints before the full solution.
- Before moving on: build, run tests (and sanitizers), suggest a git commit
  message. Keep commits small.
- End each stage with: a short recap, 3-5 interview questions about that
  stage, and an updated "Current status" section below.
- Never silently skip correctness for speed. Every optimized version must
  pass the differential test.

## Current status

**Stage 0 complete and pushed** (`ece54f2` on
`github.com/GitArya05/HFT-Engine`). Skeleton created: folder structure,
`CMakeLists.txt` + `CMakePresets.json` (MSVC + Ninja, `debug`/`release`
presets), GoogleTest v1.15.2 + Google Benchmark v1.9.1 via FetchContent,
placeholder smoke-test targets (`hft_server`, `hft_tests`, `hft_bench`) to
prove the build+test+bench loop, `.gitignore`, `.gitattributes`,
`.clang-format`, GitHub Actions CI (Windows build+test; Linux TSan job
deferred to Stage 4), README skeleton, this file.

Verified end to end on this machine: both presets configure, build clean
from scratch, and pass 1/1 test; benchmark and server executables run.

Three Windows/MSVC gotchas found while verifying (all already worked
around, don't rediscover them):

1. CMake's `-G "Visual Studio 17 2022"` **cannot find this VS install**
   ("could not find any instance of Visual Studio"). Use Ninja + vcvars.
2. MSVC `/fsanitize=address` changes the ABI of STL types via container
   annotations, so linking ASan objects against a non-ASan GoogleTest or
   Benchmark fails with `LNK2038 ... 'annotate_*'`. The individual
   `_DISABLE_*_ANNOTATION` macros are a moving target across MSVC versions
   (`annotate_optional` was not covered by the two documented ones), so the
   ASan flag is applied **globally** via `add_compile_options()` before
   `FetchContent_MakeAvailable()` and every dependency is built with it.
3. ASan-instrumented exes need `clang_rt.asan_dynamic-x86_64.dll` from the
   MSVC toolset folder on `PATH` at **runtime**. Launched from a plain
   terminal they die instantly with `STATUS_DLL_NOT_FOUND`
   (exit `-1073741515`) and no message. Run them via `scripts/run.ps1`.

Google Benchmark's configure log reports `HAVE_PTHREAD_AFFINITY`,
`HAVE_POSIX_REGEX` and `CMAKE_HAVE_LIBC_PTHREAD` as failed — expected on
Windows, it falls back to its Win32 paths. Not an error.

**Next:** Stage 1, Step 1 — design `include/hft/types.hpp` (the `Price`,
`Qty`, `OrderId`, `Side` type aliases) and the `Order` struct.
