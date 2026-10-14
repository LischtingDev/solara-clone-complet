#include <cassert>
#include <cstdio>
#include <vector>

#include "ipc/protocol.hpp"

using namespace sc::ipc;

int main() {
    // Encode/decode simple.
    Message m{};
    m.type = MsgType::Execute;
    m.payload = R"({"source":"print(1)","name":"a.luau"})";

    auto bytes = encode(m);
    assert(bytes.size() == 4 + 1 + m.payload.size());

    Message out{};
    size_t consumed = 0;
    assert(try_decode(bytes.data(), bytes.size(), out, consumed));
    assert(consumed == bytes.size());
    assert(out.type == MsgType::Execute);
    assert(out.payload == m.payload);

    // Buffer incomplet : false, rien consomme.
    consumed = 123;
    assert(!try_decode(bytes.data(), 3, out, consumed));

    // Buffer partiel (entete ok, payload tronque).
    assert(!try_decode(bytes.data(), bytes.size() - 2, out, consumed));

    // Message vide.
    Message e{};
    e.type = MsgType::Ping;
    auto eb = encode(e);
    assert(try_decode(eb.data(), eb.size(), out, consumed));
    assert(out.type == MsgType::Ping);
    assert(out.payload.empty());

    // Deux messages concatenes : le premier doit etre decode.
    std::vector<uint8_t> two = bytes;
    two.insert(two.end(), eb.begin(), eb.end());
    assert(try_decode(two.data(), two.size(), out, consumed));
    assert(consumed == bytes.size());
    assert(out.type == MsgType::Execute);

    std::puts("test_ipc_framing: OK");
    return 0;
}