#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ipc/protocol.hpp"

namespace sc::ipc {

// Serveur de pipe (cote core injecte). Un client a la fois, re-armable.
class PipeServer {
public:
    PipeServer() = default;
    ~PipeServer();
    PipeServer(const PipeServer&) = delete;
    PipeServer& operator=(const PipeServer&) = delete;

    bool create(std::wstring_view name, size_t buffer = 1u << 20);
    bool accept(int timeout_ms);
    void disconnect();
    bool connected() const { return connected_; }

    // Bloque jusqu'a un message complet ou expiration du timeout.
    bool read(Message& out, int timeout_ms);
    bool write(const Message& msg, int timeout_ms = 2000);

private:
    HANDLE               pipe_ = INVALID_HANDLE_VALUE;
    HANDLE               ev_   = nullptr;
    std::vector<uint8_t> rx_;
    bool                 connected_ = false;
};

// Client de pipe (cote UI). Fourni aussi en C++ pour les outils.
class PipeClient {
public:
    PipeClient() = default;
    ~PipeClient();
    PipeClient(const PipeClient&) = delete;
    PipeClient& operator=(const PipeClient&) = delete;

    bool connect(std::wstring_view name, int timeout_ms);
    void close();

    bool read(Message& out, int timeout_ms);
    bool write(const Message& msg, int timeout_ms = 2000);

private:
    HANDLE               pipe_ = INVALID_HANDLE_VALUE;
    HANDLE               ev_   = nullptr;
    std::vector<uint8_t> rx_;
};

} // namespace sc::ipc