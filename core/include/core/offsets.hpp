#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "core/scanner.hpp"

namespace sc::core {

struct Offsets {
    bool ok = false;

    // VM Luau
    uintptr_t luau_load        = 0;
    uintptr_t luaD_throw       = 0;

    // API Luau (remplies par signature, voir docs/OFFSETS.md)
    uintptr_t lua_pcall        = 0;
    uintptr_t lua_getglobal    = 0;
    uintptr_t lua_setglobal    = 0;
    uintptr_t lua_pushcclosurek= 0;
    uintptr_t lua_createtable  = 0;

    // Roblox
    uintptr_t script_context   = 0;
    uintptr_t get_global_state = 0;

    std::vector<std::string> missing;
};

const std::vector<Signature>& signature_table();

// Charge offsets.json s'il existe, sinon scanne, puis complete par scan.
Offsets resolve_offsets();

// Chemin du fichier d'override (a cote de la DLL).
std::wstring offsets_path();

} // namespace sc::core