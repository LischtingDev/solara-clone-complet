// Attache a un Roblox live et ecrit les offsets resolus.
// Usage : dump_offsets --pid 1234 --out core/offsets.json
#include <windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "core/scanner.hpp"

namespace {

struct RemoteModule { uintptr_t base; size_t size; };

RemoteModule find_module(uint32_t pid, const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return {};

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    RemoteModule out{};
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, name) == 0) {
                out.base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                out.size = me.modBaseSize;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return out;
}

std::vector<uint8_t> read_remote(HANDLE proc, uintptr_t addr, size_t size) {
    std::vector<uint8_t> buf(size);
    SIZE_T got = 0;
    if (!ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(addr), buf.data(), size, &got))
        buf.resize(got);
    return buf;
}

} // namespace

int main(int argc, char** argv) {
    uint32_t pid = 0;
    std::string out_path = "offsets.json";

    for (int i = 1; i < argc; ++i) {
        const std::string k = argv[i];
        if (k == "--pid" && i + 1 < argc) pid = static_cast<uint32_t>(std::atoi(argv[++i]));
        else if (k == "--out" && i + 1 < argc) out_path = argv[++i];
    }

    if (pid == 0) { std::puts("usage: dump_offsets --pid <n> [--out file]"); return 2; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (proc == nullptr) { std::puts("OpenProcess a echoue"); return 1; }

    auto mod = find_module(pid, L"RobloxPlayerBeta.exe");
    if (mod.base == 0) { std::puts("module RobloxPlayerBeta.exe introuvable"); return 1; }
    std::printf("module @ 0x%llx, taille 0x%zx\n",
                static_cast<unsigned long long>(mod.base), mod.size);

    auto blob = read_remote(proc, mod.base, mod.size);
    std::printf("lu %zu octets\n", blob.size());

    // TODO(J7) : utiliser core::signature_table() et ecrire le JSON complet.
    // Pour l'instant on valide juste la lecture et un pattern d'exemple.
    auto p = sc::core::compile_pattern("48 89 5C 24 ?? 48 89 6C 24 ??");
    if (p) {
        if (auto h = sc::core::match_in_buffer(*p, blob.data(), blob.size()))
            std::printf("exemple: hit @ 0x%llx\n",
                        static_cast<unsigned long long>(mod.base + *h));
        else
            std::puts("exemple: aucun hit");
    }

    std::ofstream out(out_path);
    out << "{\n  \"_roblox_version\": \"a remplir\",\n";
    out << "  \"Luau::luau_load\": \"0x0\"\n}\n";
    std::printf("ecrit %s\n", out_path.c_str());

    CloseHandle(proc);
    return 0;
}