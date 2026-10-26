# QuestBridge
Experimental open-source foundations for investigating standalone headsets with native PS5 VR games.

**No PS VR2 game compatibility is implemented or demonstrated.** This repository currently provides a tested desktop communication prototype, native data/math/video utilities, and untested Android and PS5 probe scaffolds. Resident Evil Village VR Mode is an untested reference target.

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
- headset/android-openxr/: Gradle/JNI OpenXR runtime probe scaffold, **unbuilt and untested**.
- ps5/host/: SDK smoke test, **not a network host**, no ELF produced.
- ps5/runtime/: unavailable adapter and explicitly named offline mock.
- docs/research/RESEARCH.md: pinned upstream findings and unresolved VR interfaces.
- docs/architecture/ARCHITECTURE.md: implemented boundaries, risks and milestones.
- docs/testing/HARDWARE.md: evidence needed before device claims.
- docs/testing/SESSION.md: actual validation results.

Android needs Android SDK/NDK, compatible JDK and Gradle; PS5 needs a POSIX toolchain and the selected homebrew SDK. See each scaffold README. There is no installable APK or PS5 ELF in this milestone.

License: GPL-3.0-or-later for original project code. Upstream projects remain under their respective terms; no upstream implementation or proprietary Sony content is bundled.

Latest validation: 9 Node tests, 1 native CTest and a C++/Node payload fixture pass locally. The Windows demo measured about63 tracking updates/s against a nominal90Hz setting; see docs/testing/SESSION.md.
