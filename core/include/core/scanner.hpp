#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace sc::core {

// Signature IDA-style : "48 8B 05 ?? ?? ?? ?? 48 85 C0"
// '?' ou '??' = wildcard d'un octet.
struct Signature {
    std::string_view name;
    std::string_view pattern;
    int32_t  offset      = 0;
    uint8_t  indirection = 0;
    bool     required    = true;
};

struct Resolved {
    const Signature* sig = nullptr;
    uintptr_t        address = 0;
};

struct CompiledPattern {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;   // 0xFF = exact, 0x00 = wildcard
};

std::optional<CompiledPattern> compile_pattern(std::string_view pattern);

// Matching pur, testable hors process.
std::optional<size_t> match_in_buffer(const CompiledPattern& p,
                                      const uint8_t* data, size_t size);

// Scanne un module charge dans CE process.
class Scanner {
public:
    explicit Scanner(std::wstring_view module_name);

    bool valid() const { return base_ != 0; }
    uintptr_t base() const { return base_; }
    size_t    size() const { return size_; }

    std::optional<uintptr_t> find(std::string_view pattern) const;
    std::optional<uintptr_t> resolve(const Signature& sig) const;
    std::vector<Resolved>    resolve_all(const std::vector<Signature>& sigs) const;

private:
    uintptr_t base_ = 0;
    size_t    size_ = 0;
};

// Lecture d'un pointeur dans le process courant (avec SEH).
std::optional<uintptr_t> safe_read_ptr(uintptr_t addr);
std::optional<uintptr_t> follow_rip_relative(uintptr_t at, int32_t offset, uint8_t indirection);

} // namespace sc::core