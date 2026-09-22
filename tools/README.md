# tools/

Standalone executables that aren't part of the engine itself or its tests:

- `order_gen` (Stage 2) — deterministic random order-stream generator
  (`std::mt19937`, fixed seed), shared by the differential test and the
  benchmarks so every run is reproducible.
- `book_cli` (Stage 1) — terminal tool for manually poking an order book
  during development.

Empty until Stage 1.
