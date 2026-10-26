// SPDX-License-Identifier: GPL-3.0-or-later
#include "transport/receiver.hpp"
#include "transport/config.hpp"
#include "ps5/runtime/bridge.hpp"
#include <iostream>
#include <limits>
#include <thread>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
template<class F> void rejects(F f) { try { f(); } catch (const std::exception&) { return; } throw std::runtime_error("Expected rejection"); }
using namespace qb::transport;
int main() {
    try {
        Key key{}; key.fill(9); Codec codec(key);
        Packet source{Type::tracking, 7, 3, 10, qb::encode_tracking({})};
        const auto encoded = codec.encode(source);
        const auto decoded = codec.decode(encoded.data(), encoded.size());
        CHECK(decoded.session == 7 && decoded.sequence == 3 && decoded.payload == source.payload);
        for (std::size_t n = 0; n < encoded.size(); ++n) rejects([&] { codec.decode(encoded.data(), n); });
        for (std::size_t i = 0; i < encoded.size(); ++i) { auto b = encoded; b[i] ^= 1; rejects([&] { codec.decode(b.data(), b.size()); }); }
        auto large = encoded; large.resize(1201); rejects([&] { codec.decode(large.data(), large.size()); });
        Codec wrong(Key{}); rejects([&] { wrong.decode(encoded.data(), encoded.size()); });
        rejects([&] { key_from_hex("short"); });
        rejects([&] { key_from_hex(std::string(64, 'x').c_str()); });
        const auto key_line = std::string("key=") + std::string(64, '1');
        const auto config = parse_config(key_line + "\r\nport=0\r\nrun_ms=200\r\n");
        CHECK(config.port == 0 && config.run_ms == 200 && config.bind == "127.0.0.1");
        for (const auto& bad_config : {std::string("port=1"), key_line + "\nport=1\nport=2", key_line + "\nunknown=1",
             key_line + "\nrun_ms=0", key_line + "\nport=65536", key_line + "\nbind=$(secret)", key_line + "\nport=-1"})
            rejects([&] { parse_config(bad_config); });
        rejects([&] { parse_config(std::string(2049, 'x')); });
        rejects([&] { parse_config(key_line + std::string(1, '\0')); });
        const Endpoint endpoint{42, 1234}, other{42, 1235};
        std::uint64_t next_id = 100;
        Receiver receiver(key, [&] { return ++next_id; }, 2000, 100);
        const auto send = [&](Packet p, Endpoint e, std::uint64_t now) {
            const auto bytes = codec.encode(p); return receiver.receive(bytes.data(), bytes.size(), e, now);
        };
        Packet hello{Type::hello, 0, 1, 0, std::vector<std::uint8_t>(16, 3)};
        const auto welcome_bytes = send(hello, endpoint, 10); CHECK(welcome_bytes.has_value());
        const auto welcome = codec.decode(welcome_bytes->data(), welcome_bytes->size()); CHECK(welcome.session == 101);
        CHECK(send(hello, endpoint, 11).has_value()); CHECK(!send(hello, other, 12));
        Packet tracking{Type::tracking, welcome.session, 1, 1000, qb::encode_tracking({})};
        CHECK(!send(tracking, other, 13)); CHECK(receiver.stats().tracking == 0);
        send(tracking, endpoint, 20); CHECK(receiver.stats().tracking == 1);
        const auto baseline = receiver.stats().accepted;
        send(tracking, endpoint, 21); CHECK(receiver.stats().accepted == baseline);
        Packet bad = tracking; bad.sequence = 2; bad.payload.assign(156, 0);
        send(bad, endpoint, 22); CHECK(receiver.stats().accepted == baseline);
        tracking.sequence = 2; send(tracking, endpoint, 23); CHECK(receiver.stats().tracking == 2);
        bad = tracking; bad.sequence = 3; bad.type = Type::welcome;
        send(bad, endpoint, 24); CHECK(receiver.stats().accepted == baseline + 1);
        bad = tracking; bad.sequence = 3; bad.sender_time_us = 999;
        send(bad, endpoint, 25); CHECK(receiver.stats().accepted == baseline + 1);
        tracking.sequence = 4; send(tracking, endpoint, 30); CHECK(receiver.stats().gaps == 1);
        CHECK(receiver.snapshot(130).fresh); CHECK(!receiver.snapshot(131).fresh);
        CHECK(receiver.snapshot(131).tracking.devices[0].flags == 0);
        Packet ping{Type::ping, welcome.session, 5, 1001, std::vector<std::uint8_t>(8)};
        write_be(ping.payload.data(), 900, 8);
        const auto pong_bytes = send(ping, endpoint, 40); CHECK(pong_bytes.has_value());
        const auto pong = codec.decode(pong_bytes->data(), pong_bytes->size());
        CHECK(pong.type == Type::pong && read_be(pong.payload.data(), 8) == 900 && read_be(pong.payload.data() + 8, 8) == 40);
        Receiver timed(key, [] { return std::uint64_t(1); }, 2000, 100, [] { return std::uint64_t(100); });
        const auto h = codec.encode(hello); timed.receive(h.data(), h.size(), endpoint, 10);
        auto timed_ping = ping; timed_ping.session = 1; timed_ping.sequence = 1;
        const auto pbytes = codec.encode(timed_ping);
        const auto timed_reply = timed.receive(pbytes.data(), pbytes.size(), endpoint, 20);
        const auto timed_pong = codec.decode(timed_reply->data(), timed_reply->size());
        CHECK(read_be(timed_pong.payload.data() + 8, 8) == 20);
        CHECK(read_be(timed_pong.payload.data() + 16, 8) == 100 && timed_pong.sender_time_us == 100);
        receiver.expire(2041); CHECK(!receiver.paired() && receiver.stats().expired == 1);
        CHECK(!receiver.snapshot(2041).fresh && receiver.snapshot(2041).tracking.devices[1].buttons == 0);
        const auto fresh_welcome_bytes = send(hello, endpoint, 2050); CHECK(fresh_welcome_bytes.has_value());
        CHECK(codec.decode(fresh_welcome_bytes->data(), fresh_welcome_bytes->size()).session != welcome.session);
        send(tracking, endpoint, 2051); CHECK(receiver.stats().tracking == 3); // Old session rejected.
        qb::Tracking t{}; t.devices[0].flags = 12; t.devices[0].pose.position.x = 3;
        qb::TrackingStore store(100); store.publish(t, 1, 1, 0, 10);
        const auto snapshot = store.sample(20); CHECK(snapshot.fresh && snapshot.tracking.devices[0].flags == 0);
        CHECK(snapshot.tracking.devices[0].pose.position.x == 0);
        rejects([&] { store.publish(t, 1, 1, 1, 11); });
        CHECK(!store.sample(9).fresh);
        // Consumer and producer may run on different threads. Frames must not tear.
        qb::TrackingStore concurrent(1000000);
        std::thread producer([&] { for (std::uint64_t i = 1; i <= 1000; ++i) { auto frame = t; for (auto& d : frame.devices) d.trigger = float(i) / 1000; concurrent.publish(frame, 2, i, i, i); } });
        for (unsigned i = 0; i < 1000; ++i) { const auto s = concurrent.sample(1000); if (s.fresh) CHECK(s.tracking.devices[0].trigger == s.tracking.devices[2].trigger); }
        producer.join();
        qb::UnavailablePs5Runtime runtime; CHECK(runtime.status() == qb::RuntimeStatus::unavailable);
        CHECK(!runtime.submit_tracking(concurrent.sample(1000).tracking));
        Receiver entropy_failure(key, [] { return std::uint64_t(0); });
        const auto bytes = codec.encode(hello); rejects([&] { entropy_failure.receive(bytes.data(), bytes.size(), endpoint, 1); });
        CHECK(!entropy_failure.paired());
        std::cout << "Native authenticated wire, sessions, replay, expiry, freshness and concurrent snapshots passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
