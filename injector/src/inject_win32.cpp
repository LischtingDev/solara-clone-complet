#include "injector/inject.hpp"
#include "injector/process.hpp"

#include <TlHelp32.h>
#include <chrono>
#include <cstring>
#include <thread>

namespace sc::injector {

namespace {

struct RemotePath {
    uintptr_t addr = 0;
    size_t    bytes = 0;
};

RemotePath write_remote_string(HANDLE proc, const std::wstring& s) {
    const size_t bytes = (s.size() + 1) * sizeof(wchar_t);
    void* mem = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (mem == nullptr) return {};
    if (!WriteProcessMemory(proc, mem, s.c_str(), bytes, nullptr)) {
        VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
        return {};
    }
    return { reinterpret_cast<uintptr_t>(mem), bytes };
}

bool run_remote_thread(HANDLE proc, uintptr_t fn, uintptr_t arg, DWORD* exit_code) {
    const auto start = reinterpret_cast<LPTHREAD_START_ROUTINE>(fn);
    HANDLE th = CreateRemoteThread(proc, nullptr, 0, start,
                                   reinterpret_cast<LPVOID>(arg), 0, nullptr);
    if (th == nullptr) return false;
    WaitForSingleObject(th, 15000);
    if (exit_code) GetExitCodeThread(th, exit_code);
    CloseHandle(th);
    return true;
}

} // namespace

InjectResult inject_dll(uint32_t pid, const std::wstring& dll_path) {
    InjectResult res{};

    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (proc == nullptr) {
        res.last_error = GetLastError();
        res.detail = "OpenProcess a echoue";
        return res;
    }

    const RemotePath rp = write_remote_string(proc, dll_path);
    if (rp.addr == 0) {
        res.last_error = GetLastError();
        res.detail = "WriteProcessMemory a echoue";
        CloseHandle(proc);
        return res;
    }

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    const uintptr_t load_library = reinterpret_cast<uintptr_t>(
        GetProcAddress(k32, "LoadLibraryW"));

    DWORD code = 0;
    if (!run_remote_thread(proc, load_library, rp.addr, &code) || code == 0) {
        res.last_error = GetLastError();
        res.detail = "CreateRemoteThread(LoadLibraryW) a echoue";
        VirtualFreeEx(proc, reinterpret_cast<LPVOID>(rp.addr), 0, MEM_RELEASE);
        CloseHandle(proc);
        return res;
    }

    // Laisse le loader mapper la DLL puis recupere sa base distante.
    uintptr_t base = 0;
    for (int i = 0; i < 100 && base == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (auto b = remote_module_base(pid, L"sc_core.dll")) base = *b;
    }

    res.ok = base != 0;
    res.module_base = base;
    if (!res.ok) res.detail = "DLL mappee mais module introuvable";

    VirtualFreeEx(proc, reinterpret_cast<LPVOID>(rp.addr), 0, MEM_RELEASE);
    CloseHandle(proc);
    return res;
}

bool call_remote_export(uint32_t pid,
                        const std::wstring& dll_path,
                        const char* export_name,
                        const std::wstring& wide_arg) {
    // Charge la DLL localement pour calculer l'offset (RVA) de l'export.
    HMODULE local = LoadLibraryW(dll_path.c_str());
    if (local == nullptr) return false;
    FARPROC fn = GetProcAddress(local, export_name);
    if (fn == nullptr) { FreeLibrary(local); return false; }

    const uintptr_t rva = reinterpret_cast<uintptr_t>(fn) -
                          reinterpret_cast<uintptr_t>(local);

    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (proc == nullptr) { FreeLibrary(local); return false; }

    auto base_opt = remote_module_base(pid, L"sc_core.dll");
    if (!base_opt) { CloseHandle(proc); FreeLibrary(local); return false; }

    const RemotePath arg = write_remote_string(proc, wide_arg);
    if (arg.addr == 0) { CloseHandle(proc); FreeLibrary(local); return false; }

    const bool ok = run_remote_thread(proc, *base_opt + rva, arg.addr, nullptr);

    // Le thread a lu la chaine : on peut liberer. Le core en garde une copie.
    VirtualFreeEx(proc, reinterpret_cast<LPVOID>(arg.addr), 0, MEM_RELEASE);
    CloseHandle(proc);
    FreeLibrary(local);
    return ok;
}

void cleanup_remote(uint32_t, uintptr_t) {}

} // namespace sc::injector