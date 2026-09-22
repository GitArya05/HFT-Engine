# Zero-Allocation, Ultra-Low-Latency Matching Engine

A single-symbol ("SYM1") limit order-matching engine, built to demonstrate
systems-engineering skills for backend / systems / performance-engineering
interviews: zero-allocation hot paths, cache-friendly data layout, lock-free
concurrency, raw networking, and rigorous latency benchmarking.

**This is not a real trading product.** The prices, the symbol, and the
order flow are fictional; the point is the engineering underneath.

## Status

See [`CLAUDE.md`](CLAUDE.md) for the full project context, design decisions,
stage plan, and current status.

## Architecture

```
React + Vite dashboard  <-->  FastAPI gateway  <-->  binary TCP  <-->  C++ engine
     (Stage 9)                  (Stage 8)            protocol       (Stages 0-7)
```

The C++ engine is the core of the project and is built and benchmarked
completely on its own first. The gateway and dashboard are a thin,
off-hot-path layer added afterward — the engine's correctness and latency
numbers are never measured through them.

## Building (Windows, MSVC + Ninja)

```powershell
./scripts/build.ps1 -Preset debug -Test
./scripts/build.ps1 -Preset release
```

Or open the folder in VS Code with the CMake Tools extension, which detects
the MSVC kit and sets up the build environment automatically.

## Layout

See `CLAUDE.md` for what lives in each folder and why.

## Results

Benchmark tables and per-stage design notes land in `docs/` starting Stage 2,
and get rolled up here at Stage 7.
