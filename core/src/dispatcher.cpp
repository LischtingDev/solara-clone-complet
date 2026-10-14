#include "core/dispatcher.hpp"
#include "core/executor.hpp"
#include "core/log.hpp"
#include "ipc/pipe.hpp"

#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace sc::core::dispatcher {

namespace {

std::atomic<bool> g_stop{false};
sc::ipc::PipeServer g_server;
std::mutex g_write_mutex;

void send(sc::ipc::MsgType type, nlohmann::json payload) {
    sc::ipc::Message m{};
    m.type = type;
    m.payload = payload.dump();

    std::scoped_lock lk(g_write_mutex);
    if (g_server.connected() && !g_server.write(m, 1000)) {
        // best effort : on ne bloque pas le core pour l'UI
    }
}

void handle(const sc::ipc::Message& m) {
    using sc::ipc::MsgType;

    nlohmann::json j = nlohmann::json::object();
    if (!m.payload.empty()) {
        try { j = nlohmann::json::parse(m.payload); } catch (...) {}
    }

    switch (m.type) {
        case MsgType::Ping:
            send(MsgType::Pong, {});
            break;

        case MsgType::Execute: {
            const std::string src  = j.value("source", "");
            const std::string name = j.value("name", "chunk");
            const auto r = execute(src, name);
            send(MsgType::ExecuteDone, { {"ok", r.ok}, {"error", r.error} });
            break;
        }

        case MsgType::ClearOutput:
            log::clear_tail();
            break;

        case MsgType::Shutdown:
            g_stop = true;
            break;

        default:
            log::warn("message inconnu: 0x{:02x}", static_cast<unsigned>(m.type));
            break;
    }
}

} // namespace

void push_output(const std::string& level, const std::string& text) {
    send(sc::ipc::MsgType::Output, { {"level", level}, {"text", text} });
}

void run(const std::wstring& pipe_name) {
    log::set_sink([](log::Level lvl, const std::string& msg) {
        const char* level =
            lvl == log::Level::Error ? "error" :
            lvl == log::Level::Warn  ? "warn"  :
            lvl == log::Level::Lua   ? "lua"   : "info";
        push_output(level, msg);
    });

    if (!g_server.create(std::wstring(L"\\\\.\\pipe\\") + pipe_name)) {
        log::error("creation du pipe impossible");
        return;
    }
    log::info("dispatcher pret sur \\\\.\\pipe\\{}", std::string(pipe_name.begin(), pipe_name.end()));

    while (!g_stop) {
        if (!g_server.accept(500)) continue;

        log::info("client connecte");
        send(sc::ipc::MsgType::AttachState,
             { {"attached", true}, {"pid", GetCurrentProcessId()} });

        sc::ipc::Message m{};
        while (!g_stop && g_server.read(m, 500)) {
            handle(m);
        }

        log::info("client deconnecte");
        g_server.disconnect();
    }
}

void stop() { g_stop = true; }

} // namespace sc::core::dispatcher