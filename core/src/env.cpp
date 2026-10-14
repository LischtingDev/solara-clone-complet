#include "core/env.hpp"
#include "core/offsets.hpp"
#include "core/log.hpp"

#include <windows.h>
#include <fstream>
#include <sstream>

namespace sc::core {

namespace {

Env g_env;

// int f(lua_State* L) : lit __sc_console(line, level) cote Lua.
int console_bridge(uintptr_t L) {
    auto& api = g_env.api;
    if (api.tostring == nullptr) return 0;
    const char* line  = api.tostring(L, 1);
    const char* level = api.tostring(L, 2);
    console_callback(line ? line : "", level ? level : "lua");
    return 0;
}

// int f(lua_State* L) : wrapper C qui relaie vers le bridge.
int print_stub(uintptr_t L) {
    return console_bridge(L);
}

std::string read_text_file(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::wstring dll_dir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p{buf};
    const auto slash = p.find_last_of(L'\\');
    return slash == std::wstring::npos ? L"." : p.substr(0, slash);
}

} // namespace

Env& env() { return g_env; }

void console_callback(const std::string& line, const std::string& level) {
    log::write(level == "error" ? log::Level::Error
             : level == "warn"  ? log::Level::Warn
                                : log::Level::Lua, line);
}

bool build(LuaStateRef st) {
    const auto offs = resolve_offsets();
    auto& api = g_env.api;

    // TODO(J4) : brancher les pointeurs reels une fois les signatures validees.
    // Les typedefs doivent etre ajustes aux conventions d'appel exactes de Roblox.
    api.pcall   = reinterpret_cast<decltype(api.pcall)>(offs.lua_pcall);
    api.getglobal = reinterpret_cast<decltype(api.getglobal)>(offs.lua_getglobal);
    api.setglobal = reinterpret_cast<decltype(api.setglobal)>(offs.lua_setglobal);
    api.pushcclosure = reinterpret_cast<decltype(api.pushcclosure)>(offs.lua_pushcclosurek);
    api.createtable  = reinterpret_cast<decltype(api.createtable)>(offs.lua_createtable);

    g_env.state = st;
    g_env.ready = api.pcall != nullptr && api.setglobal != nullptr;

    if (!g_env.ready) {
        log::warn("API Luau incomplete : environnement partiel");
        return false;
    }

    // Expose __sc_console (bridge C) puis execute init.luau.
    api.pushcclosure(st.state, reinterpret_cast<void*>(&print_stub), 0, "__sc_console");
    api.setglobal(st.state, "__sc_console");

    const std::wstring init_path = dll_dir() + L"\\lua\\init.luau";
    const std::string source = read_text_file(init_path);
    if (source.empty()) {
        log::warn("lua/init.luau introuvable a cote de la DLL");
        return true;
    }

    // TODO(J4) : charger et appeler via lua_load + pcall.
    log::info("env: init.luau lu ({} octets)", source.size());
    return true;
}

} // namespace sc::core