# Native authenticated diagnostic receiver

Implemented in C++17 with OpenSSL 3 HMAC-SHA256 and secure session randomness.
The same source cross-compiles with the pinned open-source PS5 SDK. It does not
call Sony VR APIs, replace headset detection, inject tracking or capture a game's
surfaces. The PS5 ELF is compilation evidence only.

## Host build and automatic tests

Requirements: C++17, CMake, OpenSSL 3 development files, POSIX sockets; Node 22+
for cross-language UDP tests. No npm packages are required.

```sh
cmake -S . -B /absolute/external/build -DCMAKE_BUILD_TYPE=Debug
cmake --build /absolute/external/build --parallel 2
ctest --test-dir /absolute/external/build --output-on-failure
npm test
```

Without OpenSSL, build the existing offline utilities using
`-DQB_BUILD_NATIVE_RECEIVER=OFF`. The PowerShell helper selects this mode so
Windows metadata builds do not acquire a new crypto dependency. The UDP executable
currently targets POSIX hosts and PS5, not Windows Winsock.

CTest covers native authentication/session semantics and a real UDP exchange
between the Node simulator and C++ receiver, including expiry and receiver restart.
The tests use synthetic poses and random ephemeral keys. They contact no console.

## Run the diagnostic on a POSIX host

```sh
export QB_KEY="$(node -e "process.stdout.write(require('node:crypto').randomBytes(32).toString('hex'))")"
QB_PORT=42420 QB_RUN_MS=10000 /absolute/external/build/qb_native_receiver
```

In another terminal, privately set the same key and run the existing simulator.
Never commit or publish the key. The default bind is `127.0.0.1`; `QB_BIND` must
explicitly name an IPv4 address to change it. `QB_PORT=0` selects an ephemeral port.
`QB_RUN_MS` defaults to 5000 and accepts 1 through 600000 milliseconds. SIGINT/SIGTERM
stop the receiver normally. The process prints its bound port and final counters;
it does not print keys, session IDs, peer addresses or pose data.

## Private file configuration

Set `QB_CONFIG` to an absolute private file path instead of using receiver environment
settings. Mixing both configuration methods is rejected. The file is bounded to
2048 bytes; unknown/duplicate settings, embedded NULs and invalid values fail before
binding. CRLF and LF are accepted. No expressions or variable expansion are evaluated.

```text
key=REPLACE_WITH_64_PRIVATE_RANDOM_HEX_DIGITS
bind=127.0.0.1
port=42420
run_ms=10000
```

The key is required. The other fields use the same loopback/port/duration defaults.
Generate the key privately; the placeholder above is deliberately invalid. Store
this file outside the repository with private permissions. The program only reads
it, and never prints its key or creates/copies it automatically.

For the PS5 build, when neither QB_CONFIG nor QB_KEY is supplied, the compiled
application attempts `/data/questbridge/receiver.conf`. This is an application
configuration path, not a firmware address. Filesystem access is unverified on the
actual loader. This mechanism does not authorize transmitting the current build-only
ELF or changing anything on the console.

## Implemented transport and tracking behavior

- Existing v1 40-byte big-endian header, 32-byte HMAC and 1200-byte datagram bound.
- OpenSSL authentication and constant-time tag comparison; no custom cryptography.
- One peer bound to IPv4 address, UDP source port, nonce and random nonzero session.
- Direction, payload/value, session and sequence checks before accepted-state updates.
- Nondecreasing sender timestamps within an accepted session; monotonic local receive time.
  Clocks are not directly compared across devices.
- PING/PONG exchange samples transmit time after validation and interoperates with the Node clock estimator; no one-way or
  motion-to-photon latency claim is made.
- Peer expiry after 2 seconds; repeated HELLO does not refresh established activity.
- Thread-safe latest tracking snapshot. Local arrival age over 100 ms returns neutral
  poses/controls with no valid/tracked flags. Missing pose-valid flags clear the
  corresponding tracked flag and neutralize that pose component.
- Expiry/reconnection clears snapshots. No extrapolation, Sony layout translation
  or XrTime conversion is attempted.
- Diagnostic 24-byte VIDEO messages are validated/counted only; no decoder, stereo
  capture or production video transport is implemented.

The Receiver state machine is single-producer; its statistics and receive/expire
methods are used on that thread. TrackingStore supports a separate concurrent
consumer. Generic control values remain independent of pose validity while a
sample is fresh; stale samples neutralize both poses and controls.

This remains experimental v1: no encryption, per-session derived keys, negotiated
capabilities, congestion control, production rate limiting or replay-protected
handshake. An authenticated old HELLO can occupy an idle peer slot until timeout.
Use private controlled testing. A reported sequence gap is not proven network loss.
Oversized UDP datagrams are rejected; the byte counter counts bytes returned to the
bounded receive buffer, not the full original length of a truncated datagram.

## PS5 compilation only

Provision the pinned SDK, LLVM C++ runtimes and cross-compiled OpenSSL as recorded
in [dependency/build evidence](../dependencies/NATIVE_TOOLCHAIN.md). Then:

```sh
export PS5_PAYLOAD_SDK=/absolute/path/to/installed-sdk
export PS5_OPENSSL_ROOT=/absolute/path/to/ps5-openssl
bash tools/build/ps5-native-host.sh /absolute/external/output
```

The script never deploys or executes the result. The default SDK CRT still performs
kernel patches before main, so this ELF is not an approved device test artifact
under the current project constraints. Target entropy, sockets, timing, threading,
stdout and loader/runtime behavior are untested on hardware. A successful link
cannot establish any of them. See the [hardware handoff](HARDWARE_NEXT.md).
