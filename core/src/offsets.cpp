#include "core/offsets.hpp"
#include "core/log.hpp"

#include <windows.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sc::core {

namespace {

// NOTE : patterns a valider avec tools/sigscan_probe. Voir docs/OFFSETS.md.
// Les entrees `required = false` degradent proprement.
const std::vector<Signature> kSignatures = {
    { "Luau::luau_load",              "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ??", 0, 0, true  },
    { "Luau::luaD_throw",             "48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57",                0, 0, false },
    { "Lua::lua_pcall",               "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B D9",          0, 0, false },
    { "Lua::lua_getglobal",           "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9",                        0, 0, false },
    { "Lua::lua_setglobal",           "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B D9",          0, 0, false },
    { "Lua::lua_pushcclosurek",       "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ??",    0, 0, false },
    { "Lua::lua_createtable",         "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9",                        0, 0, false },
    { "ScriptContext::GetGlobalState","48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9",                        0, 0, false },
};

std::wstring module_dir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p{buf};
    const auto slash = p.find_last_of(L'\\');
    return slash == std::wstring::npos ? L"." : p.substr(0, slash);
}

void apply_json(const std::wstring& path, Offsets& o) {
    std::ifstream in(path);
    if (!in) return;

    nlohmann::json j;
    try { in >> j; } catch (...) { log::warn("offsets.json illisible"); return; }

    auto get = [&](const char* key) -> uintptr_t {
        auto it = j.find(key);
        if (it == j.end() || !it->is_string()) return 0;
        return static_cast<uintptr_t>(std::strtoull(it->get<std::string>().c_str(), nullptr, 16));
    };

    o.luau_load         = get("Luau::luau_load");
    o.luaD_throw        = get("Luau::luaD_throw");
    o.lua_pcall         = get("Lua::lua_pcall");
    o.lua_getglobal     = get("Lua::lua_getglobal");
    o.lua_setglobal     = get("Lua::lua_setglobal");
    o.lua_pushcclosurek = get("Lua::lua_pushcclosurek");
    o.lua_createtable   = get("Lua::lua_createtable");
    o.get_global_state  = get("ScriptContext::GetGlobalState");

    log::info("offsets.json charge");
}

} // namespace

const std::vector<Signature>& signature_table() { return kSignatures; }

std::wstring offsets_path() { return module_dir() + L"\\offsets.json"; }

Offsets resolve_offsets() {
    Offsets o{};

    // 1) Override JSON (utile quand le scan casse).
    apply_json(offsets_path(), o);

    // 2) Scan de ce qui manque.
    Scanner sc{L"RobloxPlayerBeta.exe"};
    if (!sc.valid()) {
        log::error("module RobloxPlayerBeta.exe introuvable dans ce process");
        o.missing.push_back("module");
        return o;
    }

    struct Field { const char* name; uintptr_t* dst; };
    const Field fields[] = {
        { "Luau::luau_load",               &o.luau_load },
        { "Luau::luaD_throw",              &o.luaD_throw },
        { "Lua::lua_pcall",                &o.lua_pcall },
        { "Lua::lua_getglobal",            &o.lua_getglobal },
        { "Lua::lua_setglobal",            &o.lua_setglobal },
        { "Lua::lua_pushcclosurek",        &o.lua_pushcclosurek },
        { "Lua::lua_createtable",          &o.lua_createtable },
        { "ScriptContext::GetGlobalState", &o.get_global_state },
    };

    for (const auto& sig : kSignatures) {
        uintptr_t* dst = nullptr;
        for (const auto& f : fields) {
            if (sig.name == f.name) { dst = f.dst; break; }
        }
        if (dst == nullptr || *dst != 0) continue;  // deja fourni par le JSON

        if (auto addr = sc.resolve(sig)) {
            *dst = *addr;
            log::info("{} = 0x{:x}", sig.name, *addr);
        } else if (sig.required) {
            log::error("signature requise introuvable : {}", sig.name);
            o.missing.emplace_back(sig.name);
        } else {
            log::warn("signature optionnelle absente : {}", sig.name);
            o.missing.emplace_back(sig.name);
        }
    }

    o.ok = o.missing.empty();
    return o;
}

} // namespace sc::core