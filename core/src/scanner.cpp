#include "core/scanner.hpp"

#include <windows.h>
#include <cstring>
#include <vector>

namespace sc::core {

std::optional<CompiledPattern> compile_pattern(std::string_view pattern) {
    CompiledPattern out;
    size_t i = 0;
    while (i < pattern.size()) {
        while (i < pattern.size() && pattern[i] == ' ') ++i;
        if (i >= pattern.size()) break;

        if (pattern[i] == '?') {
            out.bytes.push_back(0x00);
            out.mask.push_back(0x00);
            while (i < pattern.size() && pattern[i] == '?') ++i;
        } else {
            auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            if (i + 1 >= pattern.size()) return std::nullopt;
            const int hi = hex(pattern[i]);
            const int lo = hex(pattern[i + 1]);
            if (hi < 0 || lo < 0) return std::nullopt;
            out.bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
            out.mask.push_back(0xFF);
            i += 2;
        }
    }
    if (out.bytes.empty()) return std::nullopt;
    return out;
}

std::optional<size_t> match_in_buffer(const CompiledPattern& p,
                                      const uint8_t* data, size_t size) {
    if (p.bytes.empty() || size < p.bytes.size()) return std::nullopt;
    const size_t last = size - p.bytes.size();
    for (size_t i = 0; i <= last; ++i) {
        bool hit = true;
        for (size_t j = 0; j < p.bytes.size(); ++j) {
            if (p.mask[j] == 0xFF && data[i + j] != p.bytes[j]) { hit = false; break; }
        }
        if (hit) return i;
    }
    return std::nullopt;
}

std::optional<uintptr_t> safe_read_ptr(uintptr_t addr) {
    __try {
        return *reinterpret_cast<uintptr_t*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return std::nullopt;
    }
}

std::optional<uintptr_t> follow_rip_relative(uintptr_t at, int32_t offset, uint8_t indirection) {
    uintptr_t p = at + offset;
    for (uint8_t i = 0; i < indirection; ++i) {
        auto v = safe_read_ptr(p);
        if (!v) return std::nullopt;
        p = *v;
    }
    return p;
}

Scanner::Scanner(std::wstring_view module_name) {
    HMODULE mod = GetModuleHandleW(std::wstring(module_name).c_str());
    if (mod == nullptr) return;

    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(mod);
    const auto nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<const uint8_t*>(mod) + dos->e_lfanew);

    base_ = reinterpret_cast<uintptr_t>(mod);
    size_ = nt->OptionalHeader.SizeOfImage;
}

std::optional<uintptr_t> Scanner::find(std::string_view pattern) const {
    if (!valid()) return std::nullopt;
    auto cp = compile_pattern(pattern);
    if (!cp) return std::nullopt;
    auto off = match_in_buffer(*cp, reinterpret_cast<const uint8_t*>(base_), size_);
    if (!off) return std::nullopt;
    return base_ + *off;
}

std::optional<uintptr_t> Scanner::resolve(const Signature& sig) const {
    auto at = find(sig.pattern);
    if (!at) return std::nullopt;
    if (sig.offset == 0 && sig.indirection == 0) return at;
    return follow_rip_relative(*at, sig.offset, sig.indirection);
}

std::vector<Resolved> Scanner::resolve_all(const std::vector<Signature>& sigs) const {
    std::vector<Resolved> out;
    out.reserve(sigs.size());
    for (const auto& s : sigs) {
        if (auto addr = resolve(s)) out.push_back({&s, *addr});
    }
    return out;
}

} // namespace sc::core