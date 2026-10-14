#include <windows.h>

#include "core/exports.hpp"
#include "core/log.hpp"
#include "core/luau_hook.hpp"
#include "core/env.hpp"
#include "core/dispatcher.hpp"
#include "ipc/protocol.hpp"

using namespace sc::core;

namespace {

const wchar_t* g_pipe_name = nullptr;
HANDLE         g_thread    = nullptr;
LuauHook       g_hook;

DWORD WINAPI bootstrap(LPVOID) {
    log::init();
    log::info("sc_core injecte, pid {}", GetCurrentProcessId());
    log::info("protocole v{}", sc::ipc::kProtocolVersion);

    g_hook.on_capture([](LuaStateRef st) {
        log::info("lua_State capture @ 0x{:x}", st.state);
        env::build(st);
    });

    if (!g_hook.install()) {
        log::error("hook Luau non installe : execution impossible");
    }

    dispatcher::run(g_pipe_name != nullptr ? g_pipe_name : L"sc_core");
    log::info("dispatcher termine");
    return 0;
}

} // namespace

extern "C" __declspec(dllexport) BOOL WINAPI Sc_Init(const wchar_t* pipe_name) {
    g_pipe_name = pipe_name;
    g_thread = CreateThread(nullptr, 0, bootstrap, nullptr, 0, nullptr);
    return g_thread != nullptr;
}

extern "C" __declspec(dllexport) void WINAPI Sc_Shutdown() {
    dispatcher::stop();
    g_hook.uninstall();
    log::shutdown();
}

extern "C" __declspec(dllexport) uint32_t WINAPI Sc_ProtocolVersion() {
    return sc::ipc::kProtocolVersion;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(nullptr);
    return TRUE;
}