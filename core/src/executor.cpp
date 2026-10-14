#include "core/executor.hpp"
#include "core/env.hpp"
#include "core/log.hpp"

#include <windows.h>
#include <chrono>

namespace sc::core {

ExecResult execute(const std::string& source, const std::string& name) {
    ExecResult res{};
    const auto t0 = std::chrono::steady_clock::now();

    if (!env().ready) {
        res.error = "environnement non pret";
        return res;
    }

    __try {
        // TODO(J4) : lua_load(source, name) + lua_pcall(0, 0, 0)
        // En attendant, on trace.
        log::info("execute '{}' ({} octets)", name, source.size());
        res.ok = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        res.ok = false;
        res.error = "exception SEH pendant l'execution";
        log::error("{}", res.error);
    }

    res.duration_us = static_cast<size_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count());
    return res;
}

} // namespace sc::core