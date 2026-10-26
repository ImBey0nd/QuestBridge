# QuestBridge cloud handoff

Updated: 2026-10-26. This file preserves the engineering context of the initial local conversation; it is a summary, not a complete chat transcript.

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
- Android Java/JNI OpenXR runtime probe scaffold; creates no XR session or tracking/rendering pipeline.
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

At handoff preparation, the repository was initialized and project files were staged, but there was no commit, remote or configured Git author. Do not assume upload occurred. Commit/push the project before expecting a remote task to see it.

The ignored .tools and build directories are local caches and must not be uploaded. Upstream README downloads are ignored; durable source SHAs and URLs are in tracked research metadata. The complete user brief is copied into tracked docs/PROJECT_BRIEF.md.

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

First reproduce the baseline in the cloud. Then provision the Android build toolchain in the cloud and attempt a real debug APK build of headset/android-openxr. The scaffold pins Android plugin 8.7.3, Gradle 8.9, JDK 17, SDK 35, NDK 27.2.12479018, CMake 3.22.1 and Khronos loader 1.1.65. Verify current official build requirements and fix concrete build failures. Preserve build logs and artifact hashes. APK compilation does not prove headset operation.

After a verified build, develop a real OpenXR session, controller actions and diagnostic projection renderer. Device execution still requires the owner's headset. In parallel milestones, the pinned ps5-payload-dev SDK can be provisioned on Linux to compile the user-space stdout probe; deployment remains separate and requires physical evidence.

Cloud work can implement, research and compile; it cannot establish hardware compatibility without authorized access to the actual devices. Update research when source accessibility changes; the provided PS5VR URL returned live 404 during the initial session despite a cached search listing.

## Prompt for the first cloud task

> Continue QuestBridge. Read AGENTS.md, docs/HANDOFF.md, docs/PROJECT_BRIEF.md and docs/testing/SESSION.md before editing. Work entirely in this cloud environment. Reproduce the existing Node/native tests, then provision the Android toolchain and build the OpenXR probe into a debug APK. Fix actual build failures and preserve evidence. Do not claim device or PS5 VR game compatibility. Continue routine edits and permitted dependency installs autonomously. Update the handoff and session results, and return a reviewable branch or pull request with the APK artifact location and remaining blockers.
