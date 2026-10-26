// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "protocol/tracking.hpp"
#include <mutex>

namespace qb {
struct TrackingSnapshot {
    Tracking tracking{};
    std::uint64_t session = 0, sequence = 0, sender_time_us = 0, received_time_us = 0, age_us = 0;
    bool fresh = false;
};
// Device-neutral local arrival clock. No Sony layout, clock conversion or pose prediction.
class TrackingStore {
    mutable std::mutex mutex_;
    TrackingSnapshot value_{};
    std::uint64_t max_age_us_;
    bool present_ = false;
public:
    explicit TrackingStore(std::uint64_t max_age_us = 100000) : max_age_us_(max_age_us) {
        if (!max_age_us || max_age_us > 1000000) throw std::invalid_argument("Tracking age limit");
    }
    void publish(const Tracking& tracking, std::uint64_t session, std::uint64_t sequence,
                 std::uint64_t sender, std::uint64_t received) {
        for (const auto& d : tracking.devices) if (!valid(d)) throw std::invalid_argument("Tracking values");
        if (!session || !sequence) throw std::invalid_argument("Tracking identity");
        std::lock_guard<std::mutex> lock(mutex_);
        if (present_ && (received < value_.received_time_us || (session == value_.session && sequence <= value_.sequence)))
            throw std::invalid_argument("Tracking order");
        value_ = {tracking, session, sequence, sender, received, 0, true}; present_ = true;
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = {}; present_ = false;
    }
    TrackingSnapshot sample(std::uint64_t now_us) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!present_ || now_us < value_.received_time_us) return {};
        auto result = value_;
        result.age_us = now_us - result.received_time_us;
        result.fresh = result.age_us <= max_age_us_;
        if (!result.fresh) { result.tracking = {}; return result; }
        for (auto& d : result.tracking.devices) {
            if (!(d.flags & 1)) { d.pose.position = {}; d.flags &= ~4u; }
            if (!(d.flags & 2)) { d.pose.orientation = {}; d.flags &= ~8u; }
        }
        return result;
    }
};
} // namespace qb
