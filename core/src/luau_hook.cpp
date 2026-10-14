#include "core/luau_hook.hpp"
#include "core/offsets.hpp"
#include "core/log.hpp"

#include <windows.h>
#include <MinHook.h>

namespace sc::core {

namespace {

// int luau_load(lua_State* L, const char* chunkname, const char* data, size_t size, int env)
using LuauLoadFn = int(__fastcall*)(uintptr_t L, const char* chunkname,
                                    const char* data, size_t size, int env);

LuauLoadFn g_original = nullptr;
LuauHook*  g_self     = nullptr;
thread_local bool t_in_hook = false;

int __fastcall detour_luau_load(uintptr_t L, const char* chunkname,
                                const char* data, size_t size, int env) {
    if (!t_in_hook && g_self != nullptr && !g_self->captured() && L != 0) {
        t_in_hook = true;
        LuaStateRef ref{};
        ref.state = L;
        g_self->on_capture_internal(ref);
        t_in_hook = false;
    }
    return g_original(L, chunkname, data, size, env);
}

} // namespace

bool LuauHook::install() {
    if (MH_Initialize() != MH_OK) {
        log::warn("MH_Initialize a echoue (peut etre deja initialise)");
    }

    g_self = this;

    const auto offs = resolve_offsets();
    if (offs.luau_load == 0) {
        log::error("luau_load introuvable : impossible d'installer le hook");
        return false;
    }

    void* target = reinterpret_cast<void*>(offs.luau_load);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&detour_luau_load),
                      reinterpret_cast<void**>(&g_original)) != MH_OK) {
        log::error("MH_CreateHook a echoue");
        return false;
    }
    if (MH_EnableHook(target) != MH_OK) {
        log::error("MH_EnableHook a echoue");
        return false;
    }

    log::info("hook luau_load installe @ 0x{:x}", offs.luau_load);
    return true;
}

void LuauHook::uninstall() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

} // namespace sc::core