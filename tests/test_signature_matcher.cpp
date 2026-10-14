#include <cassert>
#include <cstdio>

#include "core/scanner.hpp"

using namespace sc::core;

int main() {
    const uint8_t data[] = {0xAA,0xBB,0xCC,0x48,0x8B,0x05,0x11,0x22,0x33,0x44,0xDD};

    auto p = compile_pattern("48 8B 05 ?? ?? ?? ??");
    assert(p.has_value());
    auto hit = match_in_buffer(*p, data, sizeof(data));
    assert(hit.has_value() && *hit == 3);

    auto miss = compile_pattern("FF FF");
    assert(!match_in_buffer(*miss, data, sizeof(data)).has_value());

    auto bad = compile_pattern("48 GG 05");
    assert(!bad.has_value());

    auto wild = compile_pattern("?? BB ?? 48");
    assert(wild.has_value());
    auto wh = match_in_buffer(*wild, data, sizeof(data));
    assert(wh.has_value() && *wh == 1);

    std::puts("test_signature_matcher: OK");
    return 0;
}