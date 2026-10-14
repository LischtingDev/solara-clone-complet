#pragma once
#include <string>
#include <vector>

#include "core/luau_hook.hpp"

namespace sc::core {

// Pointeurs vers l'API Luau de Roblox, resolus par signature.
// Les signatures exactes sont a identifier (docs/OFFSETS.md).
struct LuaApi {
    int  (*pcall)(uintptr_t L, int nargs, int nresults, int errfunc) = nullptr;
    int  (*loadbuffer)(uintptr_t L, const char* chunk, size_t size, const char* name) = nullptr;
    void (*getglobal)(uintptr_t L, const char* name) = nullptr;
    void (*setglobal)(uintptr_t L, const char* name) = nullptr;
    void (*pushcclosure)(uintptr_t L, void* fn, int nup, const char* name) = nullptr;
    void (*pushstring)(uintptr_t L, const char* s) = nullptr;
    void (*pushnumber)(uintptr_t L, double n) = nullptr;
    void (*pushboolean)(uintptr_t L, int b) = nullptr;
    void (*pushlightuserdata)(uintptr_t L, void* p) = nullptr;
    const char* (*tostring)(uintptr_t L, int idx) = nullptr;
    int  (*gettop)(uintptr_t L) = nullptr;
    void (*settop)(uintptr_t L, int idx) = nullptr;
    void (*createtable)(uintptr_t L, int narr, int nrec) = nullptr;
    void (*setfield)(uintptr_t L, int idx, const char* k) = nullptr;
};

// Construit l'environnement de script : globals custom, capture de print,
// puis execution de core/lua/init.luau.
bool build(LuaStateRef st);

// Fournit l'etat courant a l'executeur.
struct Env {
    LuaStateRef state{};
    LuaApi      api{};
    bool        ready = false;
};
Env& env();

// Callback C expose a Lua : __sc_console(line, level)
void console_callback(const std::string& line, const std::string& level);

} // namespace sc::core