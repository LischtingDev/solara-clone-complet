#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <string_view>

namespace sc::injector {

struct InjectResult {
    bool        ok = false;
    uintptr_t   module_base = 0;
    DWORD       last_error = 0;
    std::string detail;
};

// Injection par CreateRemoteThread + LoadLibraryW.
InjectResult inject_dll(uint32_t pid, const std::wstring& dll_path);

// Appelle un export de la DLL deja injectee, avec un argument wide-string.
// L'export doit avoir la signature : BOOL WINAPI f(const wchar_t*).
bool call_remote_export(uint32_t pid,
                        const std::wstring& dll_path,
                        const char* export_name,
                        const std::wstring& wide_arg);

// Liberation du thread/alloc distant d'une injection. Appeler apres inject_dll.
void cleanup_remote(uint32_t pid, uintptr_t remote_path_alloc);

} // namespace sc::injector