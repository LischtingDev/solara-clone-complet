#include "core/log.hpp"

#include <windows.h>
#include <chrono>
#include <cstdio>
#include <mutex>

namespace sc::core::log {

namespace {

std::mutex  g_mutex;
FILE*       g_file = nullptr;
std::deque<std::pair<Level, std::string>> g_tail;
std::function<void(Level, const std::string&)> g_sink;

constexpr size_t kTailMax = 4096;

const char* level_tag(Level l) {
    switch (l) {
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERR ";
        case Level::Lua:   return "LUA ";
    }
    return "?";
}

} // namespace

void init(const std::wstring& file_name) {
    std::scoped_lock lk(g_mutex);
    if (g_file != nullptr) return;

    wchar_t dir[MAX_PATH]{};
    GetModuleFileNameW(nullptr, dir, MAX_PATH);
    std::wstring path{dir};
    const auto slash = path.find_last_of(L'\\');
    if (slash != std::wstring::npos) path = path.substr(0, slash + 1);
    path += file_name;

    _wfopen_s(&g_file, path.c_str(), L"w");
}

void shutdown() {
    std::scoped_lock lk(g_mutex);
    if (g_file) { std::fclose(g_file); g_file = nullptr; }
}

void set_sink(std::function<void(Level, const std::string&)> sink) {
    std::scoped_lock lk(g_mutex);
    g_sink = std::move(sink);
}

void write(Level lvl, std::string_view msg) {
    std::function<void(Level, const std::string&)> sink;
    {
        std::scoped_lock lk(g_mutex);
        const std::string line = std::format("[{}] {}", level_tag(lvl), msg);
        if (g_file) { std::fputs(line.c_str(), g_file); std::fputc('\n', g_file); std::fflush(g_file); }
        g_tail.emplace_back(lvl, std::string(msg));
        while (g_tail.size() > kTailMax) g_tail.pop_front();
        sink = g_sink;
    }
    if (sink) sink(lvl, std::string(msg));
}

std::vector<std::pair<Level, std::string>> tail(size_t max_lines) {
    std::scoped_lock lk(g_mutex);
    std::vector<std::pair<Level, std::string>> out;
    const size_t n = max_lines < g_tail.size() ? max_lines : g_tail.size();
    out.reserve(n);
    for (size_t i = g_tail.size() - n; i < g_tail.size(); ++i) out.push_back(g_tail[i]);
    return out;
}

void clear_tail() {
    std::scoped_lock lk(g_mutex);
    g_tail.clear();
}

} // namespace sc::core::log