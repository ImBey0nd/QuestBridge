# Architecture and feasibility
Status: 2026-10-26. End-to-end native PS VR2 replacement feasibility is unresolved.

The intended flow is headset OpenXR tracking -> authenticated transport -> PS5 receiver -> verified VR runtime adapter -> native game. Native stereo surfaces would flow through a platform encoder -> frame transport -> Android hardware decoder -> OpenXR projection layers. The desktop substitute and the C++ native receiver for the first transport path run on a POSIX host. The receiver cross-compiles to PS5 but has not run there.
The desktop receiver consumes tracking for diagnostics; it is not a Sony runtime.

Implemented:
- Device-neutral native models, capabilities and haptic request representation.
- Rigid coordinate recentering (translation and unit quaternion inverse).
- Equivalent C++/Node tracking payload formats with explicit validation.
- Authenticated Node/C++ UDP, session lifecycle, heartbeat and four-timestamp clock messages; C++ local-age tracking snapshots neutralize stale data.
- Tiny raw synthetic stereo datagrams and bounded standalone chunk assembly.
- Firmware gate: all versions unavailable for PS VR2 operations.
- Abstract runtime, unavailable PS5 adapter and explicitly labeled offline mock.

Scaffolds:
- Android Activity/JNI loader and OpenXR instance probe only; no XR session/render loop/tracking or Android network transport.
- PS5 stdout probe and authenticated receiver compile to ELF; target execution is untested and blocked by current default-CRT constraints. Native VR module integration and live process inspection remain unimplemented.

Proposed interfaces (not demonstrated Sony integration):
HeadsetBackend produces Tracking with capabilities. VrRuntime accepts tracking only after platform evidence. A future capture interface must specify ownership, pixel format, GPU fences, stereo view configuration and presentation time before an encoder can be selected.
No codec, timing or memory-layout assumption is embedded into the common native tracking representation.
OpenXR runtime should handle optical distortion and final compositor timewarp. Remote frame prediction/reprojection needs verified pose timing and cannot be replaced by flat-screen capture.
A future Android client needs xrWaitFrame/xrBeginFrame/xrLocateViews and action spaces at predicted display time, valid/tracked flag propagation, projection swapchains and lifecycle handling. Convert XrTime through the applicable runtime time conversion extension, never reinterpret integer clocks.
Quest 2 profile should report no eye tracking, no headset vibration and no adaptive triggers. A game requiring unavailable features must receive an explicit unsupported result or a documented tested fallback.

## Firmware and compatibility
13.20 is the requested initial target, with no QuestBridge console tests. Runtime firmware discovery remains unimplemented. Descriptors express requested target and evidence separately; no patch offsets exist. Unknown versions fail closed.
The target is a reusable runtime layer for any native PS VR2 game. No reference title is required or installed for testing yet; the game evidence database is empty. Per-title records will be added after a title is selected, without invented IDs or compatibility results.
The owner has Quest 2 with original Touch controllers and no physical PS VR2. Target current Quest 2 system software; no connected PS VR2 baseline or purchase is a prerequisite for cloud development. Missing Quest 2 capabilities remain explicit per-title constraints.

## Risk register
| Risk | Impact | Next evidence |
| --- | --- | --- |
| Headset/service detection cannot be substituted | Native VR mode may never start | Lawfully obtained module import/export and service traces |
| Unknown Sony VR ABIs | Unsafe calls or process crashes | Exact build IDs, signatures, ownership and ABI verification |
| Compositor surface access/encoding unavailable | No headset image | Verified surface access and encoder timing experiment |
| Runtime privileges differ by exploit/firmware | Payload may fail | Firmware, loader and user-space console logs |
| Missing Quest 2 eye/trigger/head-haptic features | Per-title incompatibility | Feature requirement matrix and explicit fallback policy |
| Wi-Fi loss/latency | Discomfort, stale pose | Packet age/deadline, congestion and device timing measurements |
| Upstream license ambiguity | Cannot reuse implementation | Review per-file licenses; no code import by default |

## Milestones and next actions
1. Desktop communication baseline: implemented and locally tested; security/negotiation remains experimental.
2. Android runtime probe: scaffold; build after provisioning JDK 17, SDK, NDK and Gradle.
3. OpenXR tracking/actions and diagnostic renderer: pending.
4. PS5 SDK compilation: stdout and native receiver ELFs built and inspected; no console logs.
5. Native authenticated receiver: OpenSSL-backed implementation tested on host and cross-compiled with pinned runtimes/crypto. Target entropy, sockets, clocks and lifecycle require hardware.
6. VR interface experiments: blocked on verifiable ABI/service evidence; no patching work is authorized implicitly.
7. Synthetic encoded stereo/Android decode: pending; tiny raw UDP samples are only transport diagnostics.
8. First game experiment: only after runtime and capture evidence. No compatibility claim at any prior milestone.

Current hardware boundary and evidence procedure: docs/testing/HARDWARE_NEXT.md. The next native VR milestone needs actual loader/build and target module/title metadata.
