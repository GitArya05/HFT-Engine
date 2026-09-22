# client/

Python. Plain `socket` module, no frameworks — this is the low-level test
client that talks the raw binary protocol directly to the C++ engine, used
for manual testing and as a load generator for real latency measurements.

Not to be confused with `gateway/` (Stage 8), which is a FastAPI service
sitting beside the engine for the web dashboard. Benchmarks always run
through this client or `tools/order_gen`, never through the gateway.

Empty until Stage 5 (`mock_trader.py`, `load_gen.py`).
