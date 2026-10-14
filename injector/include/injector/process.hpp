#pragma once
#include <windows.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sc::injector {

struct ProcessInfo {
    uint32_t     pid = 0;
    std::wstring exe;
    std::wstring path;
};

// Tous les process dont le nom d'executable correspond (insensible a la casse).
std::vector<ProcessInfo> find_processes(std::wstring_view exe_name);

// Attend l'apparition d'un process, puis d'un uptime minimum (stabilite).
std::optional<ProcessInfo> wait_for_process(std::wstring_view exe_name,
                                            int timeout_ms,
                                            int min_uptime_ms);

// La fenetre principale existe-t-elle ? (proxy de "client pret").
bool is_window_ready(uint32_t pid);

// Base d'un module charge dans un process distant.
std::optional<uintptr_t> remote_module_base(uint32_t pid, std::wstring_view module_name);

bool enable_debug_privilege();

} // namespace sc::injector