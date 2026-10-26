// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace qb::transport {
using Key = std::array<std::uint8_t, 32>;
constexpr std::size_t header_size = 40, tag_size = 32, max_datagram = 1200;
enum class Type : std::uint16_t { hello = 1, welcome = 2, tracking = 3, ping = 4, pong = 5, video = 6 };
struct Packet {
    Type type;
    std::uint64_t session = 0, sequence = 0, sender_time_us = 0;
    std::vector<std::uint8_t> payload;
};
inline void write_be(std::uint8_t* out, std::uint64_t value, unsigned size) {
    for (unsigned i = 0; i < size; ++i) out[i] = static_cast<std::uint8_t>(value >> (8 * (size - i - 1)));
}
inline std::uint64_t read_be(const std::uint8_t* in, unsigned size) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < size; ++i) value = (value << 8) | in[i];
    return value;
}
inline Key key_from_hex(const char* text) {
    if (!text || std::strlen(text) != 64) throw std::invalid_argument("QB_KEY must contain 64 hexadecimal digits");
    const auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw std::invalid_argument("QB_KEY contains a non-hexadecimal character");
    };
    Key key{};
    for (std::size_t i = 0; i < key.size(); ++i) key[i] = static_cast<std::uint8_t>(16 * digit(text[i * 2]) + digit(text[i * 2 + 1]));
    return key;
}
inline std::uint64_t random_session() {
    std::uint64_t value = 0;
    // Fail if the provider cannot obtain entropy; never fall back to a predictable ID.
    for (unsigned i = 0; i < 4; ++i) {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) throw std::runtime_error("OpenSSL random source failed");
        if (value) return value;
    }
    throw std::runtime_error("Random session source repeatedly returned zero");
}
class Codec {
    Key key_;
    Key mac(const std::uint8_t* data, std::size_t size) const {
        Key tag{}; std::size_t written = 0;
        if (!EVP_Q_mac(nullptr, "HMAC", nullptr, "SHA256", nullptr, key_.data(), key_.size(),
                       data, size, tag.data(), tag.size(), &written) || written != tag.size())
            throw std::runtime_error("OpenSSL HMAC-SHA256 failed");
        return tag;
    }
public:
    explicit Codec(const Key& key) : key_(key) {}
    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;
    ~Codec() { OPENSSL_cleanse(key_.data(), key_.size()); }
    std::vector<std::uint8_t> encode(const Packet& packet) const {
        const auto type = static_cast<unsigned>(packet.type);
        if (type < 1 || type > 6 || packet.payload.size() > max_datagram - header_size - tag_size)
            throw std::invalid_argument("Packet type or length");
        std::vector<std::uint8_t> out(header_size + packet.payload.size() + tag_size, 0);
        std::memcpy(out.data(), "QBRG", 4);
        write_be(out.data() + 4, 1, 2); write_be(out.data() + 6, type, 2);
        write_be(out.data() + 8, packet.payload.size(), 2);
        write_be(out.data() + 12, packet.session, 8); write_be(out.data() + 20, packet.sequence, 8);
        write_be(out.data() + 28, packet.sender_time_us, 8);
        std::copy(packet.payload.begin(), packet.payload.end(), out.begin() + header_size);
        const auto tag = mac(out.data(), out.size() - tag_size);
        std::copy(tag.begin(), tag.end(), out.end() - tag_size);
        return out;
    }
    Packet decode(const std::uint8_t* data, std::size_t size) const {
        if (!data || size < header_size + tag_size || size > max_datagram) throw std::invalid_argument("Datagram length");
        if (std::memcmp(data, "QBRG", 4) || read_be(data + 4, 2) != 1) throw std::invalid_argument("Magic or version");
        const auto type = read_be(data + 6, 2);
        if (type < 1 || type > 6 || read_be(data + 10, 2) || read_be(data + 36, 4)
            || read_be(data + 8, 2) != size - header_size - tag_size) throw std::invalid_argument("Datagram header");
        const auto tag = mac(data, size - tag_size);
        if (CRYPTO_memcmp(tag.data(), data + size - tag_size, tag.size()) != 0) throw std::invalid_argument("Authentication");
        return {static_cast<Type>(type), read_be(data + 12, 8), read_be(data + 20, 8), read_be(data + 28, 8),
                std::vector<std::uint8_t>(data + header_size, data + size - tag_size)};
    }
};
} // namespace qb::transport
