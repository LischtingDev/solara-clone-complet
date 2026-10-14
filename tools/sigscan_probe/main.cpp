// Scanne un fichier sur disque (exe/dll) pour une signature IDA-style.
// Usage : sigscan_probe <fichier> "48 8B 05 ?? ?? ?? ??"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "core/scanner.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::puts("usage: sigscan_probe <fichier> \"pattern\"");
        return 2;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) { std::puts("fichier illisible"); return 1; }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    std::puts(("taille: " + std::to_string(data.size()) + " octets").c_str());

    auto p = sc::core::compile_pattern(argv[2]);
    if (!p) { std::puts("pattern invalide"); return 1; }

    size_t hits = 0;
    size_t off = 0;
    while (off < data.size()) {
        auto h = sc::core::match_in_buffer(*p, data.data() + off, data.size() - off);
        if (!h) break;
        std::printf("hit #%zu @ file+0x%zx\n", ++hits, off + *h);
        off += *h + 1;
        if (hits > 64) { std::puts("... (tronque)"); break; }
    }

    std::printf("total: %zu\n", hits);
    return hits == 1 ? 0 : (hits == 0 ? 1 : 3);
}