# QuestBridge
Experimental open-source foundations for investigating standalone headsets with native PS5 VR games.

**No PS VR2 game compatibility is implemented or demonstrated.** This repository currently provides a tested desktop communication prototype, native data/math/video utilities, a built Android OpenXR stereo/tracking client with authenticated transport (not tested on a headset), and a cross-compiled PS5 probe with audited startup limitations. The goal is a reusable layer for any native PS VR2 game, with no required reference title.

Native receiver milestone: [authenticated C++ transport and freshness handling](docs/testing/NATIVE_RECEIVER.md) now passes Node interoperability and cross-compiles to PS5. The ELF remains build-only. [Hardware evidence needed next](docs/testing/HARDWARE_NEXT.md).

Native discovery implementation: [read-only ELF import inventory](docs/testing/PS5_IMPORTS.md), with [interposition research](docs/research/PS5_INTERPOSITION.md). This identifies candidate call boundaries; no runtime hook is installed.

Latest PS5 investigation: [PS VR2 detection and initialization](docs/research/PSVR2_DETECTION.md). Public PS5VR source recovered through a fork; native headset substitution remains unimplemented. [Offline log diagnostic](docs/testing/PSVR2_LOGS.md) separates logged statuses from hardware evidence.

## Para el dueño

Empieza por [LO_QUE_TENGO_QUE_HACER_YO.md](docs/LO_QUE_TENGO_QUE_HACER_YO.md): permisos de GitHub, pruebas físicas y qué resultados devolver.

## Run the working demo
Requires Node.js 22 or newer. No npm dependencies or install step.
```powershell
npm test
npm run demo
```
The demo uses an ephemeral random key, loopback UDP and generated tracking plus 2x2 RGB images per eye. It prints tracking counts, stereo packet counts and a clock round-trip estimate, then exits. No headset or console is contacted.

## Native build on this workstation
```powershell
powershell -ExecutionPolicy Bypass -File tools/build/native.ps1
```
The script uses the locally installed portable compiler under .tools when present. Otherwise install a C++17 compiler and use:
```text
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Separate host and simulator
In one PowerShell terminal:
```powershell
$env:QB_KEY = node -e "console.log(require('node:crypto').randomBytes(32).toString('hex'))"
# Copy this key privately to the second terminal; do not commit it.
npm run host
```
In a second terminal set QB_KEY to that same generated value, then run:
```powershell
npm run simulator
```
Both default to 127.0.0.1:42420. For a private LAN experiment explicitly set QB_BIND on the host and QB_HOST on the simulator to the relevant local address. QB_PORT overrides the port. There is no discovery, NAT traversal, encryption or public service configuration. HMAC authenticates packets; it does not conceal tracking data. Ctrl+C stops either process.

## Project map
- common/: C++17 models, calibration, tracking wire codec, frame assembly; Node authenticated packet codec.
- desktop/: real UDP diagnostic receiver and synthetic sender.
- tests/: malformed packet, replay, network lifecycle and native validation checks.
- headset/android-openxr/: OpenXR stereo/tracking diagnostic client, **APK built and linted; device execution untested**.
- ps5/host/native/: authenticated C++ receiver, cross-compiled PS5 ELF; no VR integration.
- ps5/host/: cross-compiled SDK probe, **build-only** because default SDK startup patches kernel state; see docs/testing/PS5_BUILD.md.
- ps5/runtime/: unavailable adapter and explicitly named offline mock.
- docs/research/RESEARCH.md: pinned upstream findings and unresolved VR interfaces.
- docs/architecture/ARCHITECTURE.md: implemented boundaries, risks and milestones.
- docs/testing/HARDWARE.md: evidence needed before device claims.
- docs/testing/SESSION.md: actual validation results.

Android needs Android SDK/NDK, compatible JDK and Gradle; PS5 needs a POSIX toolchain and the selected homebrew SDK. See each scaffold README. The Android debug APK can be downloaded from CI artifacts or built with `tools/build/android.sh`; see [Android instructions](headset/android-openxr/README.md). The PS5 ELF is a compilation artifact, not approved for device execution.

License: GPL-3.0-or-later for original project code. Upstream projects remain under their respective terms; no upstream implementation or proprietary Sony content is bundled.

Latest cloud validation: 20 Node tests and 4 native CTests pass. Native ELF metadata and transport tests also pass AddressSanitizer/UndefinedBehaviorSanitizer; VR fixtures are synthetic. Earlier local C++/Node payload fixture validation also passed. The Windows demo measured about63 tracking updates/s against a nominal90Hz setting; see docs/testing/SESSION.md.

Android cloud build/lint and APK evidence: [CLOUD_2026-10-26.md](docs/testing/CLOUD_2026-10-26.md). The combined CI validates both the native receiver and Android probe.

Quest client 0.2.0 diagnostic scene and tracking/PC receiver procedure: [QUEST_CLIENT.md](docs/testing/QUEST_CLIENT.md). No native PS5 VR game compatibility is demonstrated.
