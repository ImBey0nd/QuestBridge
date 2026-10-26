// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5/research/elf_imports.hpp"
#include <array>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static std::string json(const std::string& text) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 32 || c >= 127) { out += "\\u00"; out += digits[c >> 4]; out += digits[c & 15]; }
        else out += static_cast<char>(c);
    }
    return out + '"';
}
static std::string hex(std::uint64_t value) {
    std::ostringstream out; out << "0x" << std::hex << value; return json(out.str());
}
int main() {
    try {
#ifdef _WIN32
        if (_setmode(_fileno(stdin), _O_BINARY) == -1) throw std::runtime_error("Cannot select binary stdin");
#endif
        std::vector<std::uint8_t> bytes;
        std::array<char, 4096> chunk{};
        while (std::cin.read(chunk.data(), chunk.size()) || std::cin.gcount()) {
            const auto count = static_cast<std::size_t>(std::cin.gcount());
            if (count > qb::research::max_elf_bytes - bytes.size()) throw std::runtime_error("ELF exceeds 64 MiB limit");
            bytes.insert(bytes.end(), chunk.data(), chunk.data() + count);
        }
        if (std::cin.bad() || (!std::cin.eof() && std::cin.fail())) throw std::runtime_error("Input read failed");
        const auto result = qb::research::inspect_imports(bytes);
        std::cout << "{\"schema_version\":1,\"scope\":\"relocation_referenced_imports\",\"elf_type\":" << hex(result.elf_type)
                  << ",\"file_size\":" << bytes.size() << ",\"needed\":[";
        bool comma = false;
        for (const auto& name : result.needed) { if (comma) std::cout << ','; comma = true; std::cout << json(name); }
        std::cout << ']';
        const auto dependencies = [](const char* key, const auto& entries) {
            std::cout << ",\"" << key << "\":[";
            bool first = true;
            for (const auto& d : entries) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << "{\"id\":" << d.id << ",\"name\":" << json(d.name) << '}';
            }
            std::cout << ']';
        };
        dependencies("needed_modules", result.needed_modules);
        dependencies("import_libraries", result.import_libraries);
        std::cout << ",\"imports\":[";
        comma = false;
        for (const auto& item : result.imports) {
            if (comma) std::cout << ',';
            comma = true;
            std::cout << "{\"symbol_index\":" << item.index << ",\"binding\":" << unsigned(item.binding)
                      << ",\"symbol_type\":" << unsigned(item.type) << ",\"raw\":" << json(item.raw) << ",\"relocations\":[";
            bool next = false;
            for (const auto& r : item.relocations) {
                if (next) std::cout << ',';
                next = true;
                std::cout << "{\"target_vaddr\":" << hex(r.address) << ",\"type\":" << r.type
                          << ",\"addend_bits\":" << hex(r.addend_bits) << ",\"segment_declares_write\":"
                          << (r.segment_declares_write ? "true" : "false") << '}';
            }
            std::cout << "]}";
        }
        std::cout << "],\"runtime_bindings_enabled\":false,\"hardware_verified\":false}\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
