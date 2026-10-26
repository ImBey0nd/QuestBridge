// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5/research/elf_imports.hpp"
#include <iostream>
#include <functional>
#include <fstream>
using Bytes = std::vector<std::uint8_t>;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
static void put(Bytes& b, std::size_t at, std::uint64_t value, unsigned width = 8) {
    for (unsigned i = 0; i < width; ++i) b.at(at + i) = static_cast<std::uint8_t>(value >> (i * 8));
}
static void text(Bytes& b, std::size_t at, const std::string& s) {
    std::copy(s.begin(), s.end(), b.begin() + static_cast<std::ptrdiff_t>(at));
    b.at(at + s.size()) = 0;
}
// Synthetic ELF64 metadata only; no Sony code, addresses or game assets.
static Bytes fixture(bool sce = false, bool gnu = false) {
    Bytes b(0x600);
    put(b, 0, 0x464c457f, 4); b[4] = 2; b[5] = 1; b[6] = 1;
    put(b, 16, sce ? 0xfe10 : 3, 2); put(b, 18, 62, 2); put(b, 20, 1, 4);
    put(b, 32, 64); put(b, 52, 64, 2); put(b, 54, 56, 2); put(b, 56, 2, 2);
    put(b, 64, sce ? 0x61000010 : 1, 4); put(b, 68, 6, 4);
    put(b, 64 + 32, b.size()); put(b, 64 + 40, b.size());
    put(b, 120, 2, 4); put(b, 128, 0x100); put(b, 136, 0x100);
    put(b, 152, 0x100); put(b, 160, 0x100);
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> tags = {
        {5, 0x300}, {10, 0x80}, {6, 0x380}, {11, 24}, {gnu ? 0x6ffffef5 : 4, 0x400},
        {23, 0x440}, {2, 48}, {20, 7}, {9, 24}, {1, 32},
        {0x61000045, (std::uint64_t(1) << 48) | 32},
        {0x61000049, (std::uint64_t(2) << 48) | 48},
    };
    std::size_t at = 0x100;
    for (const auto& t : tags) { put(b, at, t.first); put(b, at + 8, t.second); at += 16; }
    text(b, 0x301, sce ? "c812oYs7Vsc#C#B" : "sceHmd2Initialize");
    text(b, 0x320, "libSceHmd2"); text(b, 0x330, "libSceHmd2");
    put(b, 0x398, 1, 4); b[0x39c] = 0x12;
    if (gnu) {
        put(b, 0x400, 1, 4); put(b, 0x404, 1, 4); put(b, 0x408, 1, 4);
        put(b, 0x418, 1, 4); put(b, 0x41c, 1, 4);
    } else { put(b, 0x400, 1, 4); put(b, 0x404, 2, 4); }
    put(b, 0x440, 0x500); put(b, 0x448, (std::uint64_t(1) << 32) | 7);
    put(b, 0x458, 0x508); put(b, 0x460, (std::uint64_t(1) << 32) | 6);
    return b;
}
static void rejects(const Bytes& b) {
    try { (void)qb::research::inspect_imports(b); } catch (const std::exception&) { return; }
    throw std::runtime_error("Malformed fixture accepted");
}
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--write-synthetic-elf") {
            const auto b = fixture(true);
            std::ofstream output(argv[2], std::ios::binary);
            output.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
            if (!output) throw std::runtime_error("Synthetic fixture write failed");
            return 0;
        }
        if (argc != 1) throw std::runtime_error("Unexpected test arguments");
        const auto good = fixture();
        for (bool sce : {false, true}) for (bool gnu : {false, true}) {
            const auto r = qb::research::inspect_imports(fixture(sce, gnu));
            CHECK(r.imports.size() == 1 && r.imports[0].relocations.size() == 2);
            CHECK(r.imports[0].relocations[0].address == 0x500);
            CHECK(r.imports[0].relocations[1].type == 6);
            CHECK(r.needed_modules[0].id == 1 && r.import_libraries[0].id == 2);
        }
        for (std::size_t n = 0; n < good.size(); ++n) rejects(Bytes(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n)));
        unsigned case_number = 0;
        const auto mutation = [&](const std::function<void(Bytes&)>& fn) {
            ++case_number; auto b = good; fn(b);
            try { rejects(b); } catch (const std::exception&) { throw std::runtime_error("Malformed mutation accepted: " + std::to_string(case_number)); }
        };
        mutation([](Bytes& b) { b[4] = 1; });
        mutation([](Bytes& b) { b[5] = 2; });
        mutation([](Bytes& b) { put(b, 32, UINT64_MAX); });
        mutation([](Bytes& b) { put(b, 56, 257, 2); });
        mutation([](Bytes& b) { put(b, 64 + 16, UINT64_MAX); });
        mutation([](Bytes& b) { put(b, 0x404, qb::research::max_symbols + 1, 4); });
        mutation([](Bytes& b) { put(b, 0x448, (std::uint64_t(2) << 32) | 7); });
        mutation([](Bytes& b) { put(b, 0x440, 0x5fc); });
        mutation([](Bytes& b) { put(b, 0x398, 0x80, 4); });
        mutation([](Bytes& b) { put(b, 0x118, UINT64_MAX); });
        mutation([](Bytes& b) { put(b, 0x168, 25); });
        mutation([](Bytes& b) { put(b, 0x178, 1); });
        mutation([](Bytes& b) { put(b, 0x198, 0x80); });
        mutation([](Bytes& b) { put(b, 0x100, 10); }); // Duplicate DT_STRSZ.
        mutation([](Bytes& b) { put(b, 0x158, 0x442); }); // Unmapped tail or invalid table.
        mutation([](Bytes& b) { std::fill(b.begin() + 0x301, b.begin() + 0x380, 'x'); });
        auto repeated = good;
        put(repeated, 0x1c0, 7); put(repeated, 0x1c8, 0x440);
        put(repeated, 0x1d0, 8); put(repeated, 0x1d8, 48);
        CHECK(qb::research::inspect_imports(repeated).imports[0].relocations.size() == 2);
        put(repeated, 0x1c8, 0x458); rejects(repeated);
        auto defined = good; put(defined, 0x39e, 1, 2);
        CHECK(qb::research::inspect_imports(defined).imports.empty());
        auto addend = good; put(addend, 0x450, 123);
        CHECK(qb::research::inspect_imports(addend).imports[0].relocations[0].addend_bits == 123);
        auto gnu = fixture(false, true); put(gnu, 0x418, 0xffffffff, 4); rejects(gnu);
        gnu = fixture(false, true); put(gnu, 0x408, 0xffffffff, 4); rejects(gnu);
        // Deterministic malformed corpus under sanitizers; either a bounded parse or an exception is valid.
        std::uint32_t seed = 0x51ac;
        for (unsigned n = 0; n < 2000; ++n) {
            auto b = good;
            for (unsigned j = 0; j < 4; ++j) { seed = seed * 1664525u + 1013904223u; b[seed % b.size()] ^= static_cast<std::uint8_t>(seed >> 24); }
            try { (void)qb::research::inspect_imports(b); } catch (const std::exception&) {}
        }
        std::cout << "ELF metadata, stripped/SCE/GNU fixtures, truncations and malformed corpus passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
