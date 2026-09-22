# gateway/ (reserved — Stage 8)

FastAPI service. Holds one persistent TCP connection to the C++ engine
(speaking the same binary protocol as `client/`), and re-exposes it as:

- REST: `POST /orders`, `DELETE /orders/{id}`
- WebSocket: `/ws/marketdata` — book + trade stream fanned out to any
  number of dashboard clients

It is a translator, not a decision-maker: no matching logic lives here, and
it is never used for latency measurement (see `client/README.md`).

Reserved and empty until Stage 8. Do not start until Stages 0-7 are done.
