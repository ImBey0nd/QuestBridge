# QuestBridge cloud handoff

Updated: 2026-10-26 after the first Linux cloud APK milestone. This is durable project context, not a complete chat transcript. See docs/testing/CLOUD_2026-10-26.md for the new build evidence and docs/FEASIBILITY.md for the owner-facing assessment.

## Read first

1. AGENTS.md: persistent engineering constraints.
2. docs/PROJECT_BRIEF.md: the owner's original full development request.
3. README.md and docs/testing/SESSION.md: implemented features and actual test results.
4. docs/research/RESEARCH.md: pinned sources and unresolved interfaces.
5. docs/architecture/ARCHITECTURE.md and docs/protocols/V1.md: boundaries and protocol limitations.

## Objective and owner preferences

Develop an open-source compatibility system for standalone VR headsets replacing PS VR2 hardware while games execute natively on PS5. Initial requested hardware is original PS5, firmware 13.20, Relapse/KStuff, Quest 2 and Touch controllers. Resident Evil Village VR Mode is the reference title, not the whole scope. None of this hardware configuration has been tested by QuestBridge.

The owner does not program. Make routine engineering choices and continue useful independent work. Avoid repeatedly requesting permission for ordinary edits, builds, tests and permitted dependency installation. Ask only for genuinely missing credentials, physical actions or irreversible operations. Obey the active environment's approval restrictions. The owner wants cloud execution to keep their PC available.

Never invent Sony ABIs, exports, offsets or compatibility. No exploit execution, kernel writes, automatic patches or console deployment. Networking and synthetic stereo do not demonstrate native game compatibility. Keep unknown firmware disabled.

## Existing implementation

- Node built-in-only desktop UDP receiver/simulator: HMAC-SHA256 authentication, random sessions, sequence checks, tracking, controller input, heartbeat/clock estimates, expiry/reconnect and raw 2x2 RGB stereo diagnostic packets.
- C++17 shared tracking/input models, payload codec, pose recentering, bounded frame assembly and unavailable PS5 runtime/firmware gates. MockVrRuntime is explicitly an offline test double.
- Android Java/JNI OpenXR runtime probe; a debug APK now builds and passes lint in Linux cloud. Creates no XR session or tracking/rendering pipeline; no device execution yet.
- PS5 SDK C stdout probe scaffold; not a tracking receiver and not built into an ELF.
- CI, research, protocol docs, evidence procedures and untested compatibility records.

The desktop protocol is experimental: no encryption, codec negotiation, production denial-of-service protection or native transport implementation. See V1.md before extending it.

## Verified local evidence, not cloud evidence

- Nine Node tests passed, including receiver restart recovery.
- GCC 16.2.0 native build and one grouped CTest passed.
- Node-generated tracking payload round-tripped identically through C++.
- Compatibility record validation passed.
- Android JNI syntax-only check passed against published OpenXR headers and desktop JNI headers. No Android linking, Gradle build, APK or headset result.
- One Windows loopback run measured about 63 Hz against nominal 90 Hz, with 95 tracking and 95 stereo test packets in about 1.505 seconds. This is not device or display performance.
- No PS5 ELF, console execution, native stereo capture, tracking injection or game compatibility evidence.

## Git and cloud transition

The initial local handoff had no commit or remote. This cloud session started from uploaded commit b72b393cf715162dec521f811b630a13bbca3de1 with origin https://github.com/ImBey0nd/QuestBridge.git and an existing Git author. Cloud changes are on questbridge/cloud-openxr-probe. Check Git and the task's final review link for publication state; do not infer a push from a local branch name.

The upload included generated Windows build files and omitted .gitignore and CI. This milestone removes generated build files from Git and adds .gitignore, CI and the Android wrapper. Toolchains, caches, keys and APKs must stay out of Git. Upstream README downloads are ignored going forward; existing uploaded research files were preserved. Durable source SHAs/URLs and the complete user brief remain tracked.

Do not assume the full original conversation automatically transfers into a new cloud task. This document and the linked repository files are the durable context. Keep this handoff and SESSION.md current after milestones.

## Cloud environment preparation

Use a Linux cloud environment with Node 22 or newer, CMake and a C++17 compiler. Install tools there, not on the owner's Windows PC. No npm dependencies are required.

Run from the repository root:

```sh
npm test
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
node tools/diagnostics/cross-language.mjs
node tools/diagnostics/validate.mjs
npm run demo
```

Record cloud results separately from previous local evidence. Do not rely on a Windows CMake cache or the ignored portable compiler.

## Next scoped task

Baseline reproduction and APK compilation are complete. Cloud results: Node 9/9, native CTest 1/1 with GCC 14.2.0, cross-language fixture and compatibility validation passed. Synthetic loopback measured about 88.46 Hz with 133 tracking and 133 tiny stereo packets, zero rejects. Final clean Android assembly/lint passed in 43s, 49 tasks executed; lint has zero errors and five warnings. Signature verification and packaged manifest/native library inspection passed. Logs and hashes: docs/testing/evidence/cloud-2026-10-26/.

Toolchain: JDK 17.0.17+10, Gradle 8.9 checksummed wrapper, Android plugin 8.7.3, SDK/target 35/min 29, explicit Build Tools 34.0.0, NDK 27.2.12479018, CMake 3.22.1, Khronos loader 1.1.65. Target SDK was raised from 32 to fix an actual lint error. Tools live under /workspace outside the repository; source /workspace/questbridge-cloud-env.sh for the current retained environment. tools/build/android.sh clean performs clean assembly and lint. Fresh environments must recreate the tools using headset/android-openxr/README.md; do not assume cloud caches persist.

APK: /workspace/QuestBridge/artifacts/cloud-2026-10-26/questbridge-probe-debug.apk, 2,112,889 bytes, SHA256 70c1740343f49ef8ed1b1337a9b546ad374761b43be0a1e38f0593c744a9e256. It is debug signed, not installed or hardware tested.

Next device experiment: run this exact probe on Quest 2 and capture runtime discovery/lifecycle evidence per docs/testing/HARDWARE.md. Next client implementation: a real OpenXR session, controller actions and diagnostic projection renderer. The pinned ps5-payload-dev SDK can separately be provisioned on Linux to compile the user-space stdout probe; deployment remains a separate physical experiment. Prioritize evidence for real PS5 VR interfaces before treating a full streaming client as a path to game compatibility.

Cloud work can implement, research and compile; it cannot establish hardware compatibility without authorized access to the actual devices. Update research when source accessibility changes; the provided PS5VR URL returned live 404 during the initial session despite a cached search listing.

## Prompt for the next cloud task

> Continue QuestBridge. Read AGENTS.md, docs/HANDOFF.md, docs/PROJECT_BRIEF.md and docs/testing/SESSION.md first. Work in the cloud. The baseline and OpenXR runtime probe APK now build; inspect docs/testing/CLOUD_2026-10-26.md and verify the retained toolchain before reusing it. Continue the next independent client or PS5 SDK compilation milestone, record actual results, keep hardware claims separate and update the handoff. PS5 VR interfaces and game compatibility remain unverified. Explain feasibility in plain language for the owner.
