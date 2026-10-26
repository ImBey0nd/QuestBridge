// SPDX-License-Identifier: GPL-3.0-or-later
// Native diagnostic receiver. No Sony VR calls, injection or game frame capture.
#include "transport/receiver.hpp"
#include "transport/config.hpp"
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

static volatile std::sig_atomic_t stop_requested = 0;
static void stop(int) { stop_requested = 1; }
static std::uint64_t now_us() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
static unsigned setting(const char* name, unsigned fallback, unsigned maximum) {
    const char* text = std::getenv(name);
    return text ? qb::transport::parse_unsigned(text, maximum) : fallback;
}
static qb::transport::ReceiverConfig configuration() {
    const char* path = std::getenv("QB_CONFIG");
#ifdef QB_PS5_PAYLOAD
    if (!path && !std::getenv("QB_KEY")) path = "/data/questbridge/receiver.conf";
#endif
    if (path) {
        if (*path != '/') throw std::invalid_argument("QB_CONFIG must be an absolute path");
        for (const char* name : {"QB_KEY", "QB_BIND", "QB_PORT", "QB_RUN_MS"})
            if (std::getenv(name)) throw std::invalid_argument("Do not mix file and environment receiver settings");
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot read private receiver configuration");
        std::array<char, qb::transport::max_config_bytes + 1> bytes{};
        file.read(bytes.data(), bytes.size());
        const auto count = static_cast<std::size_t>(file.gcount());
        if (file.bad()) throw std::runtime_error("Configuration read failed");
        return qb::transport::parse_config(std::string_view(bytes.data(), count));
    }
    qb::transport::ReceiverConfig config;
    config.key = qb::transport::key_from_hex(std::getenv("QB_KEY"));
    config.port = setting("QB_PORT", 42420, 65535);
    config.run_ms = setting("QB_RUN_MS", 5000, 600000);
    if (!config.run_ms) throw std::invalid_argument("QB_RUN_MS must be positive");
    if (const char* address = std::getenv("QB_BIND")) config.bind = address;
    return config;
}
struct Socket {
    int fd = -1;
    ~Socket() { if (fd >= 0) ::close(fd); }
};
int main() {
    try {
        auto config = configuration();
        qb::transport::Receiver receiver(config.key, qb::transport::random_session, 2000000, 100000, now_us);
        OPENSSL_cleanse(config.key.data(), config.key.size());
        const auto port = config.port;
        const auto duration_ms = config.run_ms;
        const char* bind_address = config.bind.c_str();
        sockaddr_in local{}; local.sin_family = AF_INET; local.sin_port = htons(static_cast<std::uint16_t>(port));
#ifdef __FreeBSD__
        local.sin_len = sizeof(local);
#endif
        if (inet_pton(AF_INET, bind_address, &local.sin_addr) != 1) throw std::invalid_argument("QB_BIND must be an IPv4 address");
        Socket socket; socket.fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socket.fd < 0) throw std::runtime_error("UDP socket creation failed");
        if (::bind(socket.fd, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) throw std::runtime_error("UDP bind failed");
        socklen_t local_size = sizeof(local);
        if (getsockname(socket.fd, reinterpret_cast<sockaddr*>(&local), &local_size) != 0) throw std::runtime_error("UDP getsockname failed");
        std::signal(SIGINT, stop); std::signal(SIGTERM, stop);
        std::cout << "{\"event\":\"listening\",\"port\":" << ntohs(local.sin_port)
                  << ",\"vr_runtime\":\"unavailable\",\"game_compatibility\":\"not_demonstrated\"}" << std::endl;
        const auto start = now_us();
        std::array<std::uint8_t, qb::transport::max_datagram + 1> bytes{};
        while (!stop_requested && now_us() - start < std::uint64_t(duration_ms) * 1000) {
            pollfd item{socket.fd, POLLIN, 0};
            const int result = ::poll(&item, 1, 20);
            if (result < 0) { if (errno == EINTR) continue; throw std::runtime_error("UDP poll failed"); }
            receiver.expire(now_us());
            if (item.revents & (POLLERR | POLLHUP | POLLNVAL)) throw std::runtime_error("UDP poll error");
            if (!(item.revents & POLLIN)) continue;
            sockaddr_in remote{}; socklen_t remote_size = sizeof(remote);
            const auto size = recvfrom(socket.fd, bytes.data(), bytes.size(), 0, reinterpret_cast<sockaddr*>(&remote), &remote_size);
            if (size < 0) { if (errno == EINTR) continue; throw std::runtime_error("UDP receive failed"); }
            if (remote.sin_family != AF_INET) continue;
            const auto reply = receiver.receive(bytes.data(), static_cast<std::size_t>(size),
                {remote.sin_addr.s_addr, ntohs(remote.sin_port)}, now_us());
            if (reply && sendto(socket.fd, reply->data(), reply->size(), 0, reinterpret_cast<const sockaddr*>(&remote), remote_size)
                         != static_cast<ssize_t>(reply->size())) throw std::runtime_error("UDP reply failed");
        }
        const auto snapshot = receiver.snapshot(now_us());
        const auto& s = receiver.stats();
        std::cout << "{\"event\":\"summary\",\"accepted\":" << s.accepted << ",\"rejected\":" << s.rejected
                  << ",\"tracking\":" << s.tracking << ",\"video\":" << s.video << ",\"expired\":" << s.expired
                  << ",\"gaps\":" << s.gaps << ",\"received_bytes\":" << s.wire_bytes
                  << ",\"snapshot_fresh\":" << (snapshot.fresh ? "true" : "false")
                  << ",\"paired\":" << (receiver.paired() ? "true" : "false")
                  << ",\"vr_runtime\":\"unavailable\",\"game_compatibility\":\"not_demonstrated\"}" << std::endl;
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
