# QuestBridge
Experimental open-source foundations for investigating standalone headsets with native PS5 VR games.

**No PS VR2 game compatibility is implemented or demonstrated.** This repository provides a tested desktop communication prototype, native data/math/video utilities, an Android OpenXR probe built into a debug APK, and an unbuilt PS5 probe scaffold. The Android probe has not run on a headset. Resident Evil Village VR Mode is an untested reference target.

## Run the working demo
Requires Node.js 22 or newer. No npm dependencies or install step.
```powershell
npm test
npm run demo
```
The demo uses an ephemeral random key, loopback UDP and generated tracking plus 2x2 RGB images per eye. It prints tracking counts, stereo packet counts and a clock round-trip estimate, then exits. No headset or console is contacted.

## Native build
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
- headset/android-openxr/: Gradle/JNI OpenXR runtime probe, **APK built; headset execution untested**.
- ps5/host/: SDK smoke test, **not a network host**, no ELF produced.
- ps5/runtime/: unavailable adapter and explicitly named offline mock.
- docs/research/RESEARCH.md: pinned upstream findings and unresolved VR interfaces.
- docs/architecture/ARCHITECTURE.md: implemented boundaries, risks and milestones.
- docs/testing/HARDWARE.md: evidence needed before device claims.
- docs/testing/SESSION.md: actual validation results.

Android needs JDK 17 and the SDK/NDK packages listed in [its README](headset/android-openxr/README.md); a verified Gradle 8.9 wrapper is included. From Linux, run `tools/build/android.sh` after configuring those tools. It builds and lints `headset/android-openxr/app/build/outputs/apk/debug/app-debug.apk`. PS5 still needs its POSIX toolchain and selected homebrew SDK; no PS5 ELF has been built.

[Cloud build evidence](docs/testing/CLOUD_2026-10-26.md) records the actual APK, signature checks, hashes and logs. [Feasibility report](docs/FEASIBILITY.md) explains what the milestone means for the owner's goal. The CI workflow repeats desktop/native checks and Android build/lint, and retains an APK artifact when it runs.

License: GPL-3.0-or-later for original project code. The Android APK packages the Apache-2.0 Khronos loader and its notices; Gradle wrapper files use Apache-2.0. Other upstream projects retain their respective terms. No proprietary Sony content is bundled.

Latest cloud validation: 9 Node tests, 1 native CTest, C++/Node payload round-trip and compatibility records passed. Android debug APK assembly and lint passed (5 warnings). The Linux loopback demo measured about 88 tracking updates/s; this is synthetic desktop traffic, not headset or game performance. See [session results](docs/testing/SESSION.md).
