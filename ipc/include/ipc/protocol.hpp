#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace sc::ipc {

// Incrementer a chaque changement incompatible du format.
inline constexpr uint32_t kProtocolVersion = 1;
inline constexpr wchar_t  kPipeName[]      = L"\\\\.\\pipe\\sc_core";
inline constexpr size_t   kMaxPayload      = 64ull * 1024 * 1024;

// Framing : [uint32 payload_size][uint8 type][payload...]
// payload_size = taille de (type + payload). Little-endian.
enum class MsgType : uint8_t {
    Ping         = 0x01,
    Pong         = 0x02,
    AttachState  = 0x03,
    Execute      = 0x04,
    ExecuteDone  = 0x05,
    Output       = 0x06,
    ClearOutput  = 0x07,
    GetInstances = 0x08,
    Instances    = 0x09,
    OffsetStatus = 0x0A,
    Shutdown     = 0x0B,
};

enum class LogLevel : uint8_t { Info, Warn, Error, Lua };

struct Message {
    MsgType     type{};
    std::string payload;  // JSON, peut etre vide
};

std::vector<uint8_t> encode(const Message& msg);

// false si le buffer est incomplet (ou corrompu). `consumed` est rempli dans
// tous les cas : octets a retirer du buffer d'entree.
bool try_decode(const uint8_t* data, size_t size, Message& out, size_t& consumed);

} // namespace sc::ipc