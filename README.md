# High-Frequency Trading (HFT) Core Engine and Telemetry Dashboard

A low-latency High-Frequency Trading matching engine written in C++, paired with a real-time telemetry dashboard built with React and TypeScript.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Project Objective](#2-project-objective)
3. [System Architecture](#3-system-architecture)
4. [Core Engine (Backend)](#4-core-engine-backend)
5. [Telemetry Dashboard (Frontend)](#5-telemetry-dashboard-frontend)
6. [Communication Protocol](#6-communication-protocol)
7. [Getting Started](#7-getting-started)
8. [Optimization Roadmap](#8-optimization-roadmap)
9. [Conclusion](#9-conclusion)

---

## 1. Executive Summary

This project is a custom-built, low-latency High-Frequency Trading (HFT) matching engine written in C++, together with a real-time telemetry dashboard written in React and TypeScript.

The system simulates the core infrastructure of a proprietary trading firm. It is designed to:

- Ingest market data and trading orders.
- Maintain a live Order Book.
- Match buy and sell orders according to price.
- Stream system vitals to a frontend dashboard with no perceptible UI latency.

The project is divided into two components:

| Component | Technology | Role |
|---|---|---|
| Core Engine | C++ | Receives orders, maintains the Order Book, matches trades, tracks throughput, and serves telemetry |
| Telemetry Dashboard | React, TypeScript | Displays live market and engine data and provides an emergency halt control |

---

## 2. Project Objective

In the financial technology industry, standard software engineering practices, such as high-level interpreted languages (for example Python), garbage-collected runtimes, and heavy general-purpose databases, are often too slow for trading workloads. In this domain, a delay of a few milliseconds can translate into substantial financial loss.

This project was built to develop practical expertise in low-level system design, with emphasis on the following areas.

**Performance Engineering.** Writing C++ in which every CPU cycle and every memory allocation is treated as a cost that must be justified. The aim is predictable, minimal latency on the critical path.

**Concurrency.** Managing multiple threads without blocking. This includes the principles of lock-free programming, where threads exchange data without relying on mutual-exclusion locks that can stall execution.

**Real-Time Data Pipelines.** Moving data from a C++ backend to a web interface within milliseconds using WebSockets, so that the operator always sees the current state of the engine.

**Mission-Critical User Interface Design.** Building an engineering dashboard that prioritizes accurate, immediate information over decorative or blocking animations. The interface is intended to resemble tooling used in a production trading environment.

---

## 3. System Architecture

The system consists of two parts: the C++ Core Engine, which performs all trading logic, and the React Dashboard, which presents the engine's state to the operator.

```
[Market Data Feed] ---> [C++ Matching Engine] ---> [WebSocket Server (TCP 8084)] ---> [React Frontend]
                                  ^                                                          |
                                  |                                                          |
                                  +------------- {"command": "HALT_ENGINE"} <---------------+
```

### 3.1 Data Flow

**Step 1: Ingestion.** The engine receives trading orders. An example order is "Buy 100 shares of Apple at $150."

**Step 2: Processing.** The engine updates the Order Book with the new order and checks whether it can be matched against an existing order on the opposite side. For example, a new Buy order is compared against resting Sell orders.

**Step 3: Telemetry Broadcast.** In parallel with processing, the engine packages its vital statistics into a compact JSON payload. These include current prices, processing speed, and the total number of orders processed. The payload is transmitted over a WebSocket connection to the frontend.

**Step 4: UI Rendering.** The React dashboard parses each incoming JSON message and updates the displayed values immediately, without reloading the page.

### 3.2 Control Path

The communication channel is bidirectional. In addition to sending telemetry to the dashboard, the engine listens on the same WebSocket for emergency commands, such as the halt command issued by the Kill Switch described in [Section 5](#5-telemetry-dashboard-frontend).

---

## 4. Core Engine (Backend)

The backend is a matching engine. Conceptually, it behaves like an auctioneer: it collects offers to buy and offers to sell, and it executes a transaction whenever the two sides agree on a price.

### 4.1 The Order Book

The Order Book is a digital ledger of all active orders. It is divided into two sides:

- **Bids:** orders from participants who want to buy.
- **Asks:** orders from participants who want to sell.

Two values summarize the state of the book at any moment:

- **Best Bid:** the highest price any participant is currently willing to pay.
- **Best Ask:** the lowest price any participant is currently willing to accept.

As long as the Best Bid is lower than the Best Ask, no trade can occur, and the orders rest in the book.

### 4.2 Order Matching

Matching occurs when the prices of opposing orders meet. For example, if one participant bids $100 and another participant asks $100, the engine matches the two orders, executes a trade, and removes the matched orders from the book.

### 4.3 Technical Highlights

**Throughput Tracking.** The engine calculates the number of Messages Per Second (MPS) that it is processing. This figure is a direct measure of the engine's speed and efficiency under load.

**WebSocket Server.** The engine runs a lightweight server on TCP port 8084. The server is used to push telemetry outward to connected clients, and it also listens for incoming emergency commands, such as the Kill Switch.

---

## 5. Telemetry Dashboard (Frontend)

The dashboard is intentionally designed to reflect a true production environment. It contains no artificial loading screens, no placeholder graphs, and no blocking CSS animations. The interface renders at t = 0, meaning that data appears as soon as it is received.

### 5.1 Top Navigation and Connection Status

**Function.** Indicates whether the user interface is actively connected to the C++ engine. When the connection is healthy, the indicator reads `TCP 8084 ACTIVE`.

**Rationale.** If the connection is lost, the interface immediately changes to a red `DISCONNECTED` state. This warns the trader that the values on screen may be stale, which is critical in an environment where decisions depend on current prices.

### 5.2 Top of Book (Best Bid and Best Ask)

**Function.** Displays the topmost layer of the Order Book, known as Level 1 pricing, in a large tabular font.

**Rationale.** Traders need to know the immediate market price at a glance. The interface uses Inter with `tabular-nums`, so that every digit occupies the same horizontal width. As a result, numbers do not shift left or right as prices change rapidly, which keeps the display stable and easy to read.

### 5.3 Spread Value and Spread Percentage

**Function.** The interface computes the spread by subtracting the Best Bid from the Best Ask, and it also expresses the spread as a percentage.

**Rationale.** The spread represents the profit margin available to market makers and is an indicator of market liquidity. A tight (small) spread generally indicates a healthy, liquid market, whereas a wide spread indicates reduced liquidity or higher uncertainty.

### 5.3.1 Formula

```
Spread = Best Ask - Best Bid
```

### 5.4 Engine Telemetry (Throughput and Orders)

**Function.** Displays two engine metrics: `throughput_mps`, which is the rate of message processing, and `total_orders`, which is the cumulative number of orders processed.

**Rationale.** These metrics demonstrate that the backend is operating at high-frequency levels. They also provide visibility into the health of the lock-free queues used for communication between threads, so that any slowdown or backlog can be detected quickly.

### 5.5 Emergency Engine Halt (Kill Switch)

**Function.** A prominent red button that, when clicked, sends the payload `{"command": "HALT_ENGINE"}` back through the WebSocket to the C++ core.

**Rationale.** In algorithmic trading, a software defect can cause a system to buy or sell uncontrollably, a failure mode commonly called a "runaway algorithm." Engineers therefore require an immediate mechanism to stop the engine and limit potential financial loss. The Kill Switch provides this control directly from the dashboard.

---

## 6. Communication Protocol

All communication between the engine and the dashboard takes place over a single WebSocket connection on TCP port 8084.

| Direction | Payload | Purpose |
|---|---|---|
| Engine to Dashboard | JSON telemetry containing best bid, best ask, `throughput_mps`, and `total_orders` | Continuous live updates of market and engine state |
| Dashboard to Engine | `{"command": "HALT_ENGINE"}` | Emergency halt of the matching engine |

JSON payloads are kept small so that serialization, transmission, and parsing add minimal delay to the telemetry path.

---

## 7. Getting Started

### 7.1 Prerequisites

- A C++17 (or later) compatible compiler, such as GCC or Clang
- CMake or Make, depending on the build configuration
- Node.js and npm, for the dashboard

### 7.2 Clone the Repository

```bash
git clone https://github.com/GitArya05/HFT-Engine.git
cd HFT-Engine
```

### 7.3 Build and Run the Core Engine

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./engine
```

On startup, the engine begins serving WebSocket connections on TCP port 8084.

### 7.4 Run the Dashboard

```bash
cd <frontend-directory>
npm install
npm run dev
```

Open the dashboard in a web browser. When the engine is running, the connection indicator displays `TCP 8084 ACTIVE`.

---

## 8. Optimization Roadmap

The system is functional. However, production-grade HFT requires extreme low-level optimization. The following three phases are planned.

### 8.1 Zero Heap Allocation

**Goal.** Remove all dynamic memory allocation from the critical path. This includes `std::vector`, `new`, and `malloc`.

**Motivation.** Dynamic allocation can invoke the operating system's memory manager, whose execution time is unpredictable. Such calls can introduce pauses at exactly the moments when the engine must respond fastest. Eliminating heap allocation from the hot path makes latency deterministic.

**Intended approach.** Pre-allocate memory at startup and reuse it, for example through fixed-size buffers and memory pools.

### 8.2 CPU Core Pinning

**Goal.** Lock the Matching Engine thread to a specific CPU core.

**Motivation.** By default, the operating system scheduler may move a thread between cores or interrupt it to run other work. Each interruption causes a context switch and can evict useful data from the CPU cache. Pinning the matching thread to a dedicated core ensures that the operating system never interrupts it, reducing jitter and keeping caches warm.

### 8.3 Lock-Free Concurrency

**Goal.** Allow different threads, such as the network thread and the trading thread, to share data without using `std::mutex` locks.

**Motivation.** Mutex locks are comparatively slow and can block a thread while it waits for another thread to release the lock. In a latency-sensitive path, this waiting is unacceptable. Lock-free data structures allow threads to exchange data without blocking, which improves throughput and keeps latency stable.

**Intended approach.** Use lock-free queues, such as single-producer single-consumer ring buffers, between threads.

### 8.4 Summary

| Phase | Optimization | Problem Addressed | Expected Outcome |
|---|---|---|---|
| 1 | Zero Heap Allocation | Unpredictable pauses from dynamic memory management | Deterministic, consistent latency |
| 2 | CPU Core Pinning | Scheduler interruptions and cache eviction | Reduced jitter and
