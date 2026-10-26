// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "transport/authenticated.hpp"
#include "runtime/tracking_store.hpp"
#include <functional>
#include <limits>
#include <optional>

namespace qb::transport {
struct Endpoint {
    std::uint32_t ipv4 = 0;
    std::uint16_t port = 0;
    bool operator==(const Endpoint& other) const { return ipv4 == other.ipv4 && port == other.port; }
};
struct ReceiverStats {
    std::uint64_t accepted = 0, rejected = 0, tracking = 0, video = 0, expired = 0, gaps = 0, wire_bytes = 0;
};
class Receiver {
    Codec codec_;
    TrackingStore store_;
    std::function<std::uint64_t()> random_;
    std::uint64_t timeout_us_;
    std::function<std::uint64_t()> send_clock_;
    struct Peer {
        Endpoint endpoint;
        std::array<std::uint8_t, 16> nonce;
        std::uint64_t session, received_sequence, last_received, sender_time, sent_sequence;
    };
    std::optional<Peer> peer_;
    ReceiverStats stats_{};
    static void add(std::uint64_t& value, std::uint64_t count = 1) {
        value += std::min(count, std::numeric_limits<std::uint64_t>::max() - value);
    }
    std::uint64_t send_time(std::uint64_t received) const {
        const auto sent = send_clock_ ? send_clock_() : received;
        if (sent < received) throw std::runtime_error("Reply clock moved backwards");
        return sent;
    }
    std::vector<std::uint8_t> reply(Type type, const std::vector<std::uint8_t>& payload, std::uint64_t now) {
        if (peer_->sent_sequence == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("Reply sequence exhausted");
        return codec_.encode({type, peer_->session, ++peer_->sent_sequence, now, payload});
    }
public:
    explicit Receiver(const Key& key, std::function<std::uint64_t()> random = random_session,
                      std::uint64_t timeout_us = 2000000, std::uint64_t max_tracking_age_us = 100000, std::function<std::uint64_t()> send_clock = {})
        : codec_(key), store_(max_tracking_age_us), random_(std::move(random)), timeout_us_(timeout_us), send_clock_(std::move(send_clock)) {
        if (!random_ || !timeout_us || timeout_us > 60000000) throw std::invalid_argument("Receiver configuration");
    }
    const ReceiverStats& stats() const { return stats_; }
    TrackingSnapshot snapshot(std::uint64_t now) const { return store_.sample(now); }
    bool paired() const { return peer_.has_value(); }
    void expire(std::uint64_t now) {
        if (peer_ && now >= peer_->last_received && now - peer_->last_received > timeout_us_) {
            peer_.reset(); store_.clear(); add(stats_.expired);
        }
    }
    std::optional<std::vector<std::uint8_t>> receive(const std::uint8_t* bytes, std::size_t size,
                                                   Endpoint sender, std::uint64_t now) {
        expire(now); add(stats_.wire_bytes, size);
        try {
            const auto packet = codec_.decode(bytes, size);
            if (packet.type == Type::hello) {
                if (packet.session || packet.payload.size() != 16) throw std::invalid_argument("HELLO");
                std::array<std::uint8_t, 16> nonce{};
                std::copy(packet.payload.begin(), packet.payload.end(), nonce.begin());
                if (peer_) {
                    if (!(sender == peer_->endpoint) || nonce != peer_->nonce) throw std::invalid_argument("Receiver busy");
                } else {
                    const auto id = random_();
                    if (!id) throw std::runtime_error("Session source returned zero");
                    peer_ = Peer{sender, nonce, id, 0, now, 0, 0}; store_.clear();
                }
                return reply(Type::welcome, packet.payload, send_time(now));
            }
            if (!peer_ || !(sender == peer_->endpoint) || packet.session != peer_->session
                || !packet.sequence || packet.sequence <= peer_->received_sequence || now < peer_->last_received
                || (peer_->received_sequence && packet.sender_time_us < peer_->sender_time))
                throw std::invalid_argument("Session, endpoint, sequence or clock");
            std::optional<Tracking> tracking;
            if (packet.type == Type::tracking) tracking = decode_tracking(packet.payload.data(), packet.payload.size());
            else if (packet.type == Type::ping) { if (peer_->sent_sequence == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("Reply sequence exhausted"); if (packet.payload.size() != 8) throw std::invalid_argument("PING length"); }
            else if (packet.type == Type::video) { if (packet.payload.size() != 24) throw std::invalid_argument("Diagnostic video length"); }
            else throw std::invalid_argument("Message direction");
            if (tracking) store_.publish(*tracking, packet.session, packet.sequence, packet.sender_time_us, now);
            if (peer_->received_sequence && packet.sequence > peer_->received_sequence + 1) add(stats_.gaps, packet.sequence - peer_->received_sequence - 1);
            peer_->received_sequence = packet.sequence; peer_->last_received = now; peer_->sender_time = packet.sender_time_us;
            add(stats_.accepted);
            if (tracking) add(stats_.tracking);
            if (packet.type == Type::video) add(stats_.video);
            if (packet.type == Type::ping) {
                std::vector<std::uint8_t> payload(24); std::copy(packet.payload.begin(), packet.payload.end(), payload.begin());
                const auto sent = send_time(now);
                write_be(payload.data() + 8, now, 8); write_be(payload.data() + 16, sent, 8);
                return reply(Type::pong, payload, sent);
            }
            return std::nullopt;
        } catch (const std::invalid_argument&) { add(stats_.rejected); return std::nullopt; }
    }
};
} // namespace qb::transport
