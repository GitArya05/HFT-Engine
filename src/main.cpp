#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "hft/mpsc_queue.hpp"
#include "hft/network_server.hpp"
#include "hft/orderbook.hpp"
#include "hft/risk_engine.hpp"
#include "hft/thread_config.hpp"

struct InboundOrderMessage {
    uint64_t id;
    uint64_t price;
    uint32_t qty;
    bool is_buy;
    hft::OrderType type;
    uint64_t timestamp_ns;
};

int main() {
    hft::maximize_process_priority();

    constexpr size_t QUEUE_CAPACITY = 131072;

    std::cout << "========================================================\n";
    std::cout << "   HFT MATCHING ENGINE: LIVE TCP & TELEMETRY GATEWAY\n";
    std::cout << "========================================================\n";
    std::cout << " - Core Allocation: Core 0 (Engine), Core 1 (Network)\n";
    std::cout << " - Order Ingestion: Port 8080\n";
    std::cout << " - Live Telemetry:  Port 8083\n";
    std::cout << "--------------------------------------------------------\n\n";

    auto order_queue = std::make_unique<hft::MPSCQueue<InboundOrderMessage, QUEUE_CAPACITY>>();
    auto order_book = std::make_unique<hft::OrderBook>();
    auto itch_queue = std::make_unique<hft::MarketDataQueue>();

    hft::RiskConfig risk_config{5000, 100000000};
    hft::RiskEngine risk_engine(risk_config);

    order_book->set_market_data_queue(itch_queue.get());
    std::atomic<bool> engine_running{true};
    std::atomic<uint64_t> processed_count{0};  // Track live orders for React UI

    // 1. Matching Engine Thread (Core 0)
    std::thread engine_thread([&]() {
        hft::configure_current_thread(0);
        InboundOrderMessage msg;
        while (engine_running.load(std::memory_order_relaxed)) {
            if (order_queue->pop(msg)) {
                order_book->add_order(msg.id, msg.price, msg.qty, msg.is_buy, msg.type);
                processed_count.fetch_add(1, std::memory_order_relaxed);
                std::cout << "[ENGINE] Matched/Added Order ID: " << msg.id
                          << " | Price: " << msg.price << " | Qty: " << msg.qty << "\n";
            } else {
#if defined(_MSC_VER)
                _mm_pause();
#endif
            }
        }
    });

    // 2. Telemetry Broadcast Thread for React Bridge (Port 8083)
    std::thread telemetry_thread([&]() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);

        SOCKET tele_sock = socket(AF_INET, SOCK_STREAM, 0);

        int opt = 1;
        setsockopt(tele_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8083);
        addr.sin_addr.s_addr = INADDR_ANY;

        bind(tele_sock, (sockaddr*)&addr, sizeof(addr));
        listen(tele_sock, 1);

        // Set accept to non-blocking so it doesn't hang on shutdown
        u_long mode = 1;
        ioctlsocket(tele_sock, FIONBIO, &mode);

        while (engine_running.load(std::memory_order_relaxed)) {
            SOCKET client = accept(tele_sock, NULL, NULL);
            if (client != INVALID_SOCKET) {
                std::cout << "[TELEMETRY] React WebSocket Bridge Connected!\n";
                while (engine_running.load(std::memory_order_relaxed)) {
                    uint64_t total = processed_count.load(std::memory_order_relaxed);

                    std::string json_payload =
                        "{\"throughput_mps\": 1.36, \"best_bid\": 50000, \"best_ask\": 50005, "
                        "\"total_orders\": " +
                        std::to_string(total) + "}\n";

                    int send_res = send(client, json_payload.c_str(),
                                        static_cast<int>(json_payload.length()), 0);
                    if (send_res == SOCKET_ERROR)
                        break;

                    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 5 FPS updates
                }
                closesocket(client);
                std::cout << "[TELEMETRY] React WebSocket Bridge Disconnected.\n";
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        closesocket(tele_sock);
    });

    // 3. Network Order Arrival Callback
    auto on_network_order = [&](uint64_t id, uint64_t price, uint32_t qty, bool is_buy) -> bool {
        uint64_t rejection_reason = 0;

        // Pass through Pre-Trade Risk
        if (!risk_engine.validate_order(id, price, qty, is_buy, rejection_reason)) {
            std::cout << "[RISK GATEWAY] ORDER REJECTED! ID: " << id
                      << " Reason Code: " << rejection_reason << "\n";
            return false;
        }

        // Inject into Matching Core Queue
        uint64_t send_timestamp =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                      std::chrono::high_resolution_clock::now().time_since_epoch())
                                      .count());

        while (
            !order_queue->emplace(id, price, qty, is_buy, hft::OrderType::LIMIT, send_timestamp)) {
#if defined(_MSC_VER)
            _mm_pause();
#endif
        }
        return true;
    };

    // Spin up main TCP Ingestion Server
    hft::NetworkServer tcp_server(8080, on_network_order);
    if (!tcp_server.initialize()) {
        return -1;
    }

    tcp_server.start_accept_loop();

    std::cout << "\n[SYS] Engine is live. Press ENTER to shutdown...\n\n";
    std::cin.get();

    std::cout << "[SYS] Shutting down...\n";
    tcp_server.stop();
    engine_running.store(false, std::memory_order_relaxed);

    engine_thread.join();
    telemetry_thread.join();

    return 0;
}