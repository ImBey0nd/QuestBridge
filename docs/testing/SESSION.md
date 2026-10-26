# Session record - 2026-10-26
Created the initial repository in the empty requested workspace.
Implemented desktop authenticated UDP sessions, synthetic tracking/input, heartbeats, timestamp sampling, peer expiry/reconnect and tiny raw synthetic stereo packets.
Implemented native models, quaternion/translation calibration, tracking serialization, bounded frame assembly and fail-closed firmware/runtime adapters.
Added research, architecture, protocol documentation, hardware procedures, compatibility records and CI.
Android runtime probe and PS5 SDK stdout probe remain scaffolds.

## Actual local validation
- npm test: 9/9 passing, including real UDP pairing, expiry/reconnect, live receiver restart recovery, authentication, replay, value validation and malformed corpus.
- GNU C++17 CMake build: successful with GCC16.2.0; native target uses warnings as errors.
- CTest: 1/1 passing (grouped validation of models, tracking wire, calibration, frame assembly and firmware/runtime gates).
- Cross-language check: Node-generated 156-byte tracking fixture decoded and re-encoded identically by C++.
- Compatibility database validation: passed.
- Android JNI source: g++ -fsyntax-only passed using published OpenXR1.1.65 headers and JDK26 JNI headers. This does not validate Android linking, Gradle configuration, packaging or runtime operation.
- Loader AAR prefab metadata verified package OpenXR and module openxr_loader; scaffold links OpenXR::openxr_loader.
- Standalone loopback demo: 95 tracking +95 tiny synthetic stereo packets accepted, 0 rejected, 6 PONG replies over about1.505s; measured tracking63.11Hz, received traffic0.167Mbps, last RTT747us. Values are one local run, not device/display latency or streaming performance.
- The nominal90Hz simulator rate was not reached on this Windows timer setup. Further pacing work and measurements are required.
- No PS5 ELF, Android APK, console log, headset test or game compatibility result.
- Git initialized. No commit created because user.name/user.email were not configured. No remote created or code published.

## Current blockers
Android SDK/NDK/Gradle and a compatible JDK for the pinned Android plugin are unavailable. The installed JDK26 was used only for header syntax checking.
PS5 SDK/POSIX Clang/lld environment is absent; WSL is not installed.
No console/headset connection or physical testing was available.
The supplied PS5VR repository was inaccessible live (404), despite a cached search listing. Its reported APIs cannot be audited.
PS VR2 detection/tracking/controller ABI and native stereo surface access remain unverified.

## Next concrete task
Provision the Android build toolchain, build and test the runtime probe on Quest2, then add an OpenXR session, controller actions and diagnostic projection renderer.
Independently provision the selected PS5 SDK to compile the user-space stdout probe. Native receiver and VR integration require their own reviewed interfaces and evidence.

## PS5-first cloud investigation — 2026-10-26

Owner direction supersedes the earlier Android-first next task: investigate PS5
before making further Quest application progress.

Audited immutable public SDK startup, build wrappers, generic module-resolution
examples, and pinned Relapse/KStuff documentation. Recorded source URLs and Git
blob IDs in docs/research/PS5_SOURCE_AUDIT.json; conclusions and remaining native
VR gates are in docs/research/PS5_FEASIBILITY.md.

Found that the default selected SDK CRT writes kernel credential and syscall
permission state before main. Corrected the probe's misleading no-patches message
and device procedure; classified the artifact compilation-only. No deployment.
PS5VR GitHub API again returned404; cached search descriptions remain unverified.

Provisioned workspace-local LLVM19.1.7 and built the pinned SDK from source.
Cross-compiled the stdout probe into a 110616-byte ELF and inspected its entry,
dynamic dependencies and disassembly. Confirmed the linked patch-init call before
main. Added tools/build/ps5-probe.sh for repeatable external-output compilation and
inspection, with logs and SHA256SUMS. Artifact SHA256 and commands are recorded in
docs/testing/PS5_BUILD.md. No Sony proprietary SDK or module binaries imported.

Validation: 9/9 Node tests, native C++ build, 1/1 CTest, host C17 syntax check and
compatibility validator passed. Loopback UDP tests needed the execution tool's
supported network permission; first default-isolation run failed in its UDP files.
Initial proxy reachability and read-only apt failures were resolved through supported
network permission and workspace-local package extraction. No code was changed to
mask test failures. The tracked Windows build/ directory was preserved.

No console execution, headset result, verified Sony VR ABI, tracking injection,
native game stereo capture or game compatibility. Next: establish and audit an
exact-loader-compatible startup path without automatic kernel writes before any
physical experiment. Quest app source remains unchanged.


## PS VR2 detection milestone — 2026-10-26

Owner clarified the priority: investigate replacing PS VR2 at the native runtime
boundary. SDK startup remains a deployment constraint, not the main research goal.
Explanations are now Spanish and concise; repository artifacts remain English.

Recovered accessible PS5VR source in GitHub-identified fork mtp-24/PS5VR at
209eb7f0160000507a0ca017842941b8e8e88f46; original repository still returns 404. Reviewed
module resolution, headset/tracker/reprojection lifecycle, Sense input, VR VideoOut,
optional VrHand and firmware/license documents. Source lists13.20 library checks
and console tests only on 12.40/13.60; these are upstream claims, not our results.
Compared against AnyPS5 at bfbc55a45b18e2002695043b8531cc4b6a162406, whose debt
records explicitly identify absent initialized VR behavior. Recorded the map,
limits and immutable source blob IDs in docs/research/PSVR2_DETECTION.md and
PSVR2_INTERFACE_MAP.json. No upstream implementation or offset table imported.

Implemented bounded offline PS5VR log interpretation. Reports attempts, raw codes,
first failure, missing stages and order. A submitted-frame marker cannot erase
errors or prove acceptance. Input firmware/origin remain declarations; hardware
verification is always false. Added synthetic fixture, usage documentation and
seven research tests to npm test. No real console logs were available or invented.

Validation: npm test: 16/16, existing native C++ build and CTest: 1/1 passed. Synthetic
CLI example correctly identifies tracker initialization failure despite a later
frame marker. No code changed after the successful full test run except docs.
No firmware gate enabled; no Sony calls, game capture, tracking injection, headset
execution or console deployment. Quest app source remained unchanged.

Next: verify 13.20 module identity and game consumer behavior, and establish whether
a native runtime interception boundary can provide device state, tracking/input
and stereo output. Physical evidence remains missing; do not treat a homebrew
player's own rendered surfaces as capture from an existing game.


## Native import discovery development — 2026-10-26

Investigated public controller import interposition in PoorDS4, ps5-gyroaim and
PuckbridgePS5. The gyroaim author reports firmware 13.20 console use; our evidence
is source review only. Pinned commits and source blob IDs are recorded in
PS5_INTERPOSITION_SOURCES.json. This supports investigating runtime imports but
does not validate any VR ABI or prove another game's surfaces can be captured.

Implemented an original C++17 read-only ELF metadata reader, host-native scanner
and Node source-catalog classifier. The reader uses program headers and standard
dynamic tags, accepts bounded SysV/GNU symbol metadata, preserves each relevant
pointer relocation, and correlates SCE library/module declarations. Qualified NIDs
produce candidate names only; no runtime or firmware gate is enabled. Node hashes
the exact input snapshot sent to the native scanner. No upstream code imported,
input executed, files patched or console contacted.

Validation: 20/20 Node tests and 2/2 CTests passed. Native import tests cover stripped
standard/SCE-shaped synthetic metadata, SysV/GNU hashes, malformed table/index/
string/overflow cases, all 1536 fixture truncations and 2000 deterministic mutations.
AddressSanitizer/UndefinedBehaviorSanitizer passed. Four NID vectors matched an
offline host execution of the pinned SDK encoder. End-to-end synthetic inspection
recovered one candidate Hmd2 import and two references. Inspection of the real,
previously cross-compiled QuestBridge probe found only puts and zero VR candidates;
its SHA256 matched the recorded artifact. No PS5 VR function was executed.

One initial invalid-input test used a valid substring offset; it was corrected
to an out-of-range string index. A misaligned relocation table case justified an
explicit ELF64 alignment check. Initial Node subprocess timeout and LeakSanitizer
tracing failure under default isolation were resolved with the supported execution
permission; the checks then passed without disabling sanitizers. Full usage,
format/resource limits and evidence meaning are in docs/testing/PS5_IMPORTS.md.

Next: use authorized actual 13.20 metadata to identify the game's VR consumer path
and validate contracts before writing a native runtime shim. Physical evidence,
live module/slot identity, lifecycle semantics, tracking result layouts and GPU
buffer ownership remain missing. Quest source remains unchanged.


## Native transport milestone and hardware boundary — 2026-10-26

Continued until the next native-integration decision requires actual hardware
metadata. Implemented OpenSSL 3 HMAC-SHA256 wire encoding/decoding, a bounded
single-peer receiver, random sessions, endpoint/sequence/timestamp validation,
PING/PONG, diagnostic VIDEO counting, expiry and a thread-safe tracking snapshot
store. Poses/controls become neutral after 100 ms local age; valid pose components
are explicit. No Sony ABI or game process is involved.

Implemented POSIX UDP executable with loopback defaults, finite run duration,
private environment key, graceful stop and JSON counters. Added deterministic
native semantic tests and real Node/C++ UDP interoperability tests, including
malformed values, wrong key/endpoint, oversized datagrams, replay and receiver
restart. Invalid settings exit before binding. All tests use synthetic tracking.
Calibration normalization and nonfinite-origin checks were fixed; double arithmetic
now matches Node quaternion tolerance at tested float32 boundary cases.

Provisioned LLVM 18.1.8 libc++/libc++abi/libunwind using the pinned SDK recipe and
OpenSSL 3.5.7 from commit 8cf17aaeb4599f8af87fefd810b5b5fee90fe69e. Both target
libraries built; the C++ receiver linked into an ELF64 x86-64 PIE. Fixed a real
cross-compilation failure by including netinet/in.h explicitly. Default CRT still
contains __crt_start/__patch_init; no target ELF was run. Provenance, component
source hashes and artifact hash are in the external build-evidence.json; details
in docs/dependencies/NATIVE_TOOLCHAIN.md. No proprietary content was used.

Validation: 20/20 Node tests, 4/4 CTests and native ASan/UBSan tests passed. Added local
GitHub CI configuration with immutable action SHAs; no remote CI run claimed.
Updated Windows metadata-only build helper to avoid an OpenSSL prerequisite there.
Tracked Windows build outputs remained unchanged. Added ignore rules for local
caches, environment files and generated ELF artifacts.

The actual console model/firmware, Relapse/KStuff/loader builds and authorized
module/title metadata are now required. Public loader source shows a getpid stub
in an argument slot that other loaders use for a resolver; no generic replacement
startup was fabricated. Follow docs/testing/HARDWARE_NEXT.md. No automatic patches,
kernel writes, console deployment, real pose injection, native stereo capture or
PS VR2 game compatibility occurred. Quest app source remained unchanged.

Private receiver configuration was completed before the hardware handoff. A bounded
key=value parser supports an explicitly selected private file and a PS5-only fallback
path, with no default key, embedding, expansion or mixed environment/file settings.
Tests cover malformed, duplicate and over-limit configuration and a real local file
startup; keys never appear in program output. Target filesystem access is untested.
The native PONG path separately samples transmit time after validation; a controlled
clock test checks distinct receive/send timestamps. Final tests and the PS5
cross-build passed after that correction. The next action is hardware environment
identification, not execution of the current build-only ELF.

## Owner environment baseline and coauthor attribution — 2026-10-26

Recorded the owner's PS5 Fat / 13.20 report separately from observed hardware.
Rechecked upstream Relapse main/releases and KStuff Lite latest release/tag through
GitHub. Saved immutable source references and the release API's asset SHA256; no
release binary was downloaded, installed or executed. Actual installed versions
and loader identity remain unknown. Updated HARDWARE_NEXT.md and HANDOFF.md.

Added the requested Codex coauthor trailer to the unpublished native milestone,
then published it on codex/ps5-native-discovery without changing main. Updated
external build provenance to reference the amended source commit. This milestone
changes metadata/documentation only; existing code test results remain applicable.
Validation: JSON parsing, compatibility validator and git diff whitespace check.
No hardware validation or PS VR2 compatibility is claimed.
