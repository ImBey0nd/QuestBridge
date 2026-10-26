// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "transport/authenticated.hpp"
#include <string>
#include <string_view>
#include <set>

namespace qb::transport {
constexpr std::size_t max_config_bytes = 2048;
struct ReceiverConfig {
    Key key{};
    std::string bind = "127.0.0.1";
    unsigned port = 42420, run_ms = 5000;
};
inline unsigned parse_unsigned(std::string_view text, unsigned maximum) {
    if (text.empty()) throw std::invalid_argument("Empty numeric setting");
    unsigned value = 0;
    for (char c : text) {
        if (c < '0' || c > '9' || value > maximum / 10) throw std::invalid_argument("Invalid numeric setting");
        value = value * 10 + static_cast<unsigned>(c - '0');
        if (value > maximum) throw std::invalid_argument("Numeric setting out of range");
    }
    return value;
}
// Deliberately small key=value format: no evaluation, expansion, aliases or fallback key.
inline ReceiverConfig parse_config(std::string_view input) {
    if (input.size() > max_config_bytes || input.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid configuration size or encoding");
    ReceiverConfig config;
    std::set<std::string> seen;
    std::size_t start = 0;
    while (start < input.size()) {
        auto end = input.find('\n', start);
        if (end == std::string_view::npos) end = input.size();
        auto line = input.substr(start, end - start); start = end + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty() || line.front() == '#') continue;
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) throw std::invalid_argument("Configuration entry lacks equals sign");
        const std::string name(line.substr(0, equals));
        const auto value = line.substr(equals + 1);
        if (!seen.insert(name).second) throw std::invalid_argument("Duplicate configuration entry");
        if (name == "key") { const std::string hex(value); config.key = key_from_hex(hex.c_str()); }
        else if (name == "bind") {
            if (value.empty() || value.size() > 15 || value.find_first_not_of("0123456789.") != std::string_view::npos)
                throw std::invalid_argument("Invalid bind setting");
            config.bind = std::string(value);
        } else if (name == "port") config.port = parse_unsigned(value, 65535);
        else if (name == "run_ms") { config.run_ms = parse_unsigned(value, 600000); if (!config.run_ms) throw std::invalid_argument("Run duration must be positive"); }
        else throw std::invalid_argument("Unknown configuration entry");
    }
    if (!seen.count("key")) throw std::invalid_argument("Configuration key is required");
    return config;
}
} // namespace qb::transport
