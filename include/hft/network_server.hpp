#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

namespace hft {

class NetworkServer {
public:
    using OrderCallback =
        std::function<bool(uint64_t id, uint64_t price, uint32_t qty, bool is_buy)>;

    explicit NetworkServer(int port, OrderCallback callback)
        : port_(port), server_socket_(INVALID_SOCKET), running_(false), order_callback_(callback) {}

    ~NetworkServer() {
        stop();
    }

    bool initialize() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[NET] WSAStartup failed: " << result << "\n";
            return false;
        }

        server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket_ == INVALID_SOCKET) {
            std::cerr << "[NET] Error creating socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            return false;
        }

        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt),
                   sizeof(opt));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

        if (bind(server_socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) ==
            SOCKET_ERROR) {
            std::cerr << "[NET] Bind failed: " << WSAGetLastError() << "\n";
            closesocket(server_socket_);
            WSACleanup();
            return false;
        }

        if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[NET] Listen failed: " << WSAGetLastError() << "\n";
            closesocket(server_socket_);
            WSACleanup();
            return false;
        }

        std::cout << "[NET] High-Performance TCP Gateway listening on port " << port_ << "...\n";
        return true;
    }

    void start_accept_loop() {
        running_ = true;
        accept_thread_ = std::thread([this]() {
            while (running_) {
                sockaddr_in client_addr{};
                int client_addr_size = sizeof(client_addr);
                SOCKET client_socket = accept(
                    server_socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_size);

                if (client_socket == INVALID_SOCKET) {
                    if (!running_)
                        break;
                    continue;
                }

                std::cout << "[NET] External trading client connected via TCP socket!\n";

// Read incoming binary stream (Expected format: id(8), price(8), qty(4), is_buy(1))
#pragma pack(push, 1)
                struct RawNetworkOrder {
                    uint64_t id;
                    uint64_t price;
                    uint32_t qty;
                    uint8_t is_buy;
                };
#pragma pack(pop)

                RawNetworkOrder net_order;
                int bytes_received =
                    recv(client_socket, reinterpret_cast<char*>(&net_order), sizeof(net_order), 0);

                if (bytes_received == sizeof(RawNetworkOrder)) {
                    if (order_callback_) {
                        order_callback_(net_order.id, net_order.price, net_order.qty,
                                        net_order.is_buy != 0);
                    }
                    const char* ack = "ACK\n";
                    send(client_socket, ack, 4, 0);
                }

                closesocket(client_socket);
            }
        });
    }

    void stop() {
        running_ = false;
        if (server_socket_ != INVALID_SOCKET) {
            closesocket(server_socket_);
            server_socket_ = INVALID_SOCKET;
        }
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        WSACleanup();
    }

private:
    int port_;
    SOCKET server_socket_;
    std::atomic<bool> running_;
    std::thread accept_thread_;  // <-- Add this missing declaration
    OrderCallback order_callback_;
};

}  // namespace hft
