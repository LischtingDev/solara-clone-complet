#pragma once
#include <cstdint>
#include <functional>

namespace sc::core {

struct LuaStateRef {
    uintptr_t state       = 0;  // lua_State*
    uintptr_t main_thread = 0;  // thread principal du scheduler, si connu
};

// Hook de `luau_load` pour capturer un lua_State vivant.
// Fallback documente : `luaD_throw` (docs/OFFSETS.md).
class LuauHook {
public:
    bool install();
    void uninstall();

    bool captured() const { return captured_; }
    LuaStateRef get() const { return captured_{}; }

    void on_capture(std::function<void(LuaStateRef)> cb) { cb_ = std::move(cb); }

private:
    bool        captured_ = false;
    LuaStateRef captured_{};
    std::function<void(LuaStateRef)> cb_;
};

} // namespace sc::core