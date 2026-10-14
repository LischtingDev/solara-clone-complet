#pragma once
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace sc::core::log {

enum class Level : uint8_t { Info, Warn, Error, Lua };

void init(const std::wstring& file_name = L"sc_core.log");
void shutdown();

void write(Level lvl, std::string_view msg);

// Sink appele pour chaque ligne (utilise par le dispatcher pour pousser au pipe).
void set_sink(std::function<void(Level, const std::string&)> sink);

// Dernieres lignes, pour rattraper ce qui a ete logge avant le pipe.
std::vector<std::pair<Level, std::string>> tail(size_t max_lines = 512);
void clear_tail();

template <class... Args>
void info(std::format_string<Args...> f, Args&&... a) {
    write(Level::Info, std::format(f, std::forward<Args>(a)...));
}
template <class... Args>
void warn(std::format_string<Args...> f, Args&&... a) {
    write(Level::Warn, std::format(f, std::forward<Args>(a)...));
}
template <class... Args>
void error(std::format_string<Args...> f, Args&&... a) {
    write(Level::Error, std::format(f, std::forward<Args>(a)...));
}
template <class... Args>
void lua(std::format_string<Args...> f, Args&&... a) {
    write(Level::Lua, std::format(f, std::forward<Args>(a)...));
}

} // namespace sc::core::log