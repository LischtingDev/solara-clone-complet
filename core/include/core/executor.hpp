#pragma once
#include <string>

namespace sc::core {

struct ExecResult {
    bool        ok = false;
    std::string error;
    size_t      duration_us = 0;
};

// Execute une source Luau dans le thread principal. Protege par SEH + pcall.
ExecResult execute(const std::string& source, const std::string& name);

} // namespace sc::core