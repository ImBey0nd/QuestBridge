// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Read-only ELF metadata. Never maps executable code or accesses a process.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace qb::research {
constexpr std::size_t max_elf_bytes = 64 * 1024 * 1024;
constexpr std::uint64_t max_symbols = 262144;
constexpr std::uint64_t max_relocations = 131072;

struct Segment {
    std::uint64_t offset, address, file_size, memory_size;
    std::uint32_t flags;
};
struct Dependency { std::uint16_t id; std::string name; };
struct Relocation {
    std::uint64_t address, addend_bits;
    std::uint32_t type;
    bool segment_declares_write;
};
struct Import {
    std::uint32_t index;
    std::uint8_t binding, type;
    std::string raw;
    std::vector<Relocation> relocations;
};
struct ImportInventory {
    std::uint16_t elf_type;
    std::vector<std::string> needed;
    std::vector<Dependency> needed_modules, import_libraries;
    std::vector<Import> imports;
};

class ElfImportReader {
    const std::vector<std::uint8_t>& data_;
    std::vector<Segment> loads_;
    std::map<std::uint64_t, std::uint64_t> tags_;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> dynamic_;
    std::uint64_t str_address_ = 0, str_size_ = 0;
    [[noreturn]] static void fail(const char* message) { throw std::runtime_error(message); }
    void range(std::uint64_t offset, std::uint64_t size) const {
        if (offset > data_.size() || size > data_.size() - offset) fail("ELF file range out of bounds");
    }
    static std::uint64_t plus(std::uint64_t a, std::uint64_t b) {
        if (b > std::numeric_limits<std::uint64_t>::max() - a) fail("ELF address overflow");
        return a + b;
    }
    std::uint64_t integer(std::uint64_t offset, unsigned bytes) const {
        range(offset, bytes);
        std::uint64_t value = 0;
        for (unsigned i = 0; i < bytes; ++i) value |= std::uint64_t(data_[static_cast<std::size_t>(offset + i)]) << (8 * i);
        return value;
    }
    std::uint64_t file_offset(std::uint64_t address, std::uint64_t size) const {
        bool found = false;
        std::uint64_t result = 0;
        for (const auto& s : loads_) {
            if (address < s.address) continue;
            const auto delta = address - s.address;
            if (delta > s.file_size || size > s.file_size - delta) continue;
            const auto offset = plus(s.offset, delta);
            if (found && result != offset) fail("Ambiguous ELF virtual address mapping");
            result = offset; found = true;
        }
        if (!found) fail("ELF metadata is not fully file-backed in a supported load segment");
        range(result, size);
        return result;
    }
    bool slot_writable(std::uint64_t address) const {
        bool found = false, writable = false;
        for (const auto& s : loads_) {
            if (address < s.address) continue;
            const auto delta = address - s.address;
            if (delta <= s.memory_size && 8 <= s.memory_size - delta) {
                if (found) fail("Ambiguous relocation target mapping");
                found = true; writable = (s.flags & 2) != 0;
            }
        }
        if (!found) fail("Relocation target outside mapped memory");
        return writable;
    }
    std::uint64_t tag(std::uint64_t key) const {
        const auto it = tags_.find(key);
        if (it == tags_.end()) fail("Required standard ELF dynamic tag missing; unsupported metadata profile");
        return it->second;
    }
    std::string string_at(std::uint64_t offset) const {
        if (offset >= str_size_) fail("ELF string index out of bounds");
        const auto available = std::min<std::uint64_t>(str_size_ - offset, 257);
        const auto start = file_offset(plus(str_address_, offset), available);
        std::string result;
        for (std::uint64_t i = 0; i < available; ++i) {
            const auto c = data_[static_cast<std::size_t>(start + i)];
            if (c == 0) return result;
            result.push_back(static_cast<char>(c));
        }
        fail("Unterminated or overlong ELF symbol/dependency string");
    }
    std::uint64_t symbol_count() const {
        if (tags_.count(4)) { // SysV DT_HASH explicitly carries nchain.
            const auto offset = file_offset(tag(4), 8);
            const auto buckets = integer(offset, 4), count = integer(offset + 4, 4);
            if (!buckets || buckets > max_symbols || !count || count > max_symbols) fail("ELF hash dimensions exceed limits");
            file_offset(tag(4), 8 + 4 * (buckets + count));
            return count;
        }
        if (!tags_.count(0x6ffffef5)) fail("Cannot bound ELF symbol table without SysV or GNU hash");
        const auto base = tag(0x6ffffef5), offset = file_offset(base, 16);
        const auto buckets = integer(offset, 4), first = integer(offset + 4, 4), bloom = integer(offset + 8, 4);
        if (!buckets || buckets > max_symbols || !bloom || bloom > max_symbols || first > max_symbols) fail("GNU hash dimensions exceed limits");
        const auto bucket_address = plus(base, 16 + 8 * bloom);
        const auto bucket_offset = file_offset(bucket_address, buckets * 4);
        std::uint64_t highest = 0;
        for (std::uint64_t i = 0; i < buckets; ++i) {
            const auto value = integer(bucket_offset + i * 4, 4);
            if (value && (value < first || value >= max_symbols)) fail("GNU hash bucket index invalid");
            highest = std::max(highest, value);
        }
        if (!highest) return first;
        const auto chains = plus(bucket_address, buckets * 4);
        for (auto index = highest; index < max_symbols; ++index) {
            const auto word = integer(file_offset(plus(chains, (index - first) * 4), 4), 4);
            if (word & 1) return index + 1;
        }
        fail("GNU hash chain exceeds limit or lacks terminator");
    }
public:
    explicit ElfImportReader(const std::vector<std::uint8_t>& data) : data_(data) {}
    ImportInventory read() {
        if (data_.size() > max_elf_bytes) fail("ELF exceeds 64 MiB limit");
        range(0, 64);
        if (integer(0, 4) != 0x464c457f || integer(4, 1) != 2 || integer(5, 1) != 1 || integer(6, 1) != 1)
            fail("Expected unencrypted ELF64 little-endian input; SELF containers are unsupported");
        if (integer(18, 2) != 62 || integer(20, 4) != 1 || integer(52, 2) != 64) fail("Unsupported ELF machine/version/header size");
        const auto type = static_cast<std::uint16_t>(integer(16, 2));
        if (type != 2 && type != 3 && type != 0xfe00 && type != 0xfe10 && type != 0xfe18) fail("Unsupported ELF object type");
        const auto phoff = integer(32, 8), phnum = integer(56, 2);
        if (integer(54, 2) != 56 || phnum == 0 || phnum > 256) fail("Invalid or excessive ELF program headers");
        range(phoff, phnum * 56);
        Segment dyn{}; bool have_dynamic = false;
        for (std::uint64_t i = 0; i < phnum; ++i) {
            const auto p = phoff + i * 56, kind = integer(p, 4);
            Segment s{integer(p + 8, 8), integer(p + 16, 8), integer(p + 32, 8), integer(p + 40, 8), static_cast<std::uint32_t>(integer(p + 4, 4))};
            if (kind != 1 && kind != 2 && kind != 0x61000010) continue;
            range(s.offset, s.file_size);
            if (s.file_size > s.memory_size) fail("ELF file segment exceeds memory size");
            plus(s.address, s.memory_size);
            if (kind == 2) {
                if (have_dynamic) fail("Multiple PT_DYNAMIC segments unsupported");
                dyn = s; have_dynamic = true;
            } else loads_.push_back(s); // PT_LOAD and public SDK's PT_SCE_RELRO.
        }
        if (!have_dynamic || loads_.empty() || dyn.file_size % 16 || dyn.file_size / 16 > 65536) fail("Invalid ELF dynamic segment");
        bool terminated = false;
        for (std::uint64_t i = 0; i < dyn.file_size; i += 16) {
            const auto key = integer(dyn.offset + i, 8), value = integer(dyn.offset + i + 8, 8);
            if (key == 0) { terminated = true; break; }
            dynamic_.emplace_back(key, value);
            // Dependency/unknown metadata tags may repeat; used singleton tags may not.
            if (key == 1 || key == 0x61000045 || key == 0x61000049) continue;
            const bool singleton = key == 2 || key == 4 || key == 5 || key == 6 || key == 7 || key == 8
                || key == 9 || key == 10 || key == 11 || key == 20 || key == 23 || key == 0x6ffffef5;
            if (!tags_.emplace(key, value).second && singleton) fail("Duplicate singleton dynamic tag");
        }
        if (!terminated) fail("ELF dynamic table lacks DT_NULL terminator");
        str_address_ = tag(5); str_size_ = tag(10);
        if (!str_size_) fail("Empty ELF string table");
        file_offset(str_address_, str_size_);
        if ((tags_.count(11) && tag(11) != 24) || (tags_.count(9) && tag(9) != 24)) fail("Unsupported ELF64 symbol/RELA entry size");
        const auto symbols = symbol_count(), sym_address = tag(6);
        if (!symbols) fail("ELF symbol table lacks null symbol");
        file_offset(sym_address, symbols * 24);
        ImportInventory result{type, {}, {}, {}, {}};
        for (const auto& entry : dynamic_) {
            const auto key = entry.first, value = entry.second;
            if (key == 1) result.needed.push_back(string_at(value));
            if (key == 0x61000045 || key == 0x61000049) {
                auto& list = key == 0x61000045 ? result.needed_modules : result.import_libraries;
                const auto id = static_cast<std::uint16_t>(value >> 48);
                if (std::any_of(list.begin(), list.end(), [id](const auto& d) { return d.id == id; })) fail("Duplicate SCE dependency identifier");
                list.push_back({id, string_at(value & 0xffffffff)});
            }
        }
        std::map<std::uint32_t, Import> imports;
        std::vector<std::pair<std::uint64_t, std::uint64_t>> parsed_ranges;
        std::uint64_t relocation_count = 0;
        for (const auto& pair : {std::pair<std::uint64_t, std::uint64_t>{7, 8}, {23, 2}}) {
            if (!tags_.count(pair.first) && !tags_.count(pair.second)) continue;
            const auto address = tag(pair.first), size = tag(pair.second);
            if (size % 24 || size / 24 > max_relocations - relocation_count) fail("ELF relocation count/size exceeds limits");
            if (pair.first == 23 && tags_.count(20) && tag(20) != 7) fail("Only RELA PLT relocations are supported");
            if (!size) continue;
            if (address % 8) fail("Unaligned ELF64 RELA table");
            const auto start = file_offset(address, size), end = start + size;
            bool repeated = false;
            for (const auto& r : parsed_ranges) {
                if (start == r.first && end == r.second) repeated = true;
                else if (start < r.second && r.first < end) fail("Partially overlapping relocation tables");
            }
            if (repeated) continue;
            parsed_ranges.emplace_back(start, end);
            relocation_count += size / 24;
            for (std::uint64_t i = 0; i < size; i += 24) {
                const auto target = integer(start + i, 8), info = integer(start + i + 8, 8), addend = integer(start + i + 16, 8);
                const auto relocation_type = static_cast<std::uint32_t>(info), index = static_cast<std::uint32_t>(info >> 32);
                if (relocation_type != 1 && relocation_type != 6 && relocation_type != 7) continue;
                if (index >= symbols) fail("Relocation symbol index out of bounds");
                const auto sym = file_offset(plus(sym_address, std::uint64_t(index) * 24), 24);
                if (integer(sym + 6, 2) != 0) continue; // A definition is not an imported consumer.
                const auto name = string_at(integer(sym, 4));
                if (name.empty()) continue;
                const auto details = integer(sym + 4, 1);
                const bool writable = slot_writable(target);
                auto [it, inserted] = imports.emplace(index, Import{index, static_cast<std::uint8_t>(details >> 4), static_cast<std::uint8_t>(details & 15), name, {}});
                (void)inserted;
                it->second.relocations.push_back({target, addend, relocation_type, writable});
            }
        }
        for (auto& entry : imports) result.imports.push_back(std::move(entry.second));
        return result;
    }
};
inline ImportInventory inspect_imports(const std::vector<std::uint8_t>& data) { return ElfImportReader(data).read(); }
} // namespace qb::research
