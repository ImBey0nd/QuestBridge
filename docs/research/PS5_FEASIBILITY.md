# PS5 feasibility and startup audit

Date: 2026-10-26. Scope: native PS5 execution with an alternative headset,
initial target original PS5 / firmware 13.20 / Relapse / KStuff.
Quest application development is deferred while the PS5 prerequisites are investigated.

## Result

Follow-up: [PS VR2 detection investigation](PSVR2_DETECTION.md) recovered an accessible public PS5VR fork. The original repository remains unavailable. The owner clarified that native VR substitution is the research priority; startup redesign remains a deployment constraint.

There is source evidence for building PS5 payloads and for SDK handling of 13.20.
There is still no verified path for replacing PS VR2 device detection, feeding
external tracking/controllers to a native game, or obtaining its stereo surfaces.
A payload loader and a video player for a real PS VR2 do not establish those paths.

The most consequential new finding is that the selected SDK performs kernel writes
before our stdout-only `main`. The probe is now classified **compilation-only**.
No upstream implementation, Sony ABI, firmware offset, or proprietary binary was
imported. No exploit, payload, or console operation was executed.

## Evidence levels

- **Source verified:** statements checked in the exact public files linked below.
- **Upstream claim:** a README or search listing describes behavior without local reproduction.
- **Compilation:** pinned SDK and probe built with LLVM 19.1.7; linked startup inspected.
- **Hardware:** no console/headset connected; no game tests.

[Source manifest](PS5_SOURCE_AUDIT.json) records immutable commit URLs and Git blob
IDs for the inspected files. Blob IDs are Git identifiers, not artifact SHA256 hashes.
This is a focused audit, not a complete review of either SDK or the exploit chain.

## 1. Default SDK startup violates the intended probe boundary

SDK commit: `a7685ce8e97ce1f8a4f38253a41d54241c9124f1`.

| Source | Direct observation | Implication |
| --- | --- | --- |
| [Compiler wrapper](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/host/bin/prospero-clang) | Adds CRT startup by default; has alternate handling for LLVM 20+. | Reviewing main alone cannot characterize the linked payload. |
| [CRT](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/crt.c) | Startup initializes kernel access and calls `__patch_init` before running main. | Default startup is not limited to stdout or user-space observation. |
| [Patch initialization](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/patch.c) | Updates process credentials and syscall-permission bounds through kernel helpers. | Default execution conflicts with AGENTS.md's no-kernel-write rule. |
| [Kernel initialization](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/kernel.c) | Consumes loader-provided kernel-access arguments; includes a 13.20 case. | Source coverage exists, but exact loader compatibility and behavior remain untested. |

These are source findings, not a claim that the default runtime was executed.
Do not copy the upstream kernel offsets into QuestBridge or enable firmware support
from this table. A build-only artifact would still carry the runtime behavior.

Next experiment is offline: audit the loader contract and establish whether a
startup path can run without automatic kernel writes. Do not merely remove one
patch call and assume that initialization, dynamic linking and constructors are
then compatible. Review their transitive behavior and inspect the resulting ELF
before considering any physical experiment. No replacement startup was fabricated.

## 2. Toolchain and payload ABIs

The [SDK make configuration](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/Makefile.inc)
selects an available `llvm-config` and derives Clang, LLVM ar and lld from it.
The existing research's Clang 18 reference is not an exclusive version requirement.
The [installed toolchain include](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/host/toolchain/prospero.mk)
and [hello-world makefile](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/samples/hello_world/Makefile)
confirm this project's include path and C main convention. Our Makefile has no
upstream deployment/debug targets.

The [Rufidj module-resolution example](https://github.com/Rufidj/PS5_DEV_HOMEBREW/blob/bcd3ec8f665a1ca7e546eca8168b89616c588816/examples/ps5_sdk/sysmodule_resolve.c)
uses a different `_start` and external-argument contract. Its
[module helper](https://github.com/Rufidj/PS5_DEV_HOMEBREW/blob/bcd3ec8f665a1ca7e546eca8168b89616c588816/ps5_sdk/ps5_sdk_sysmodule_helpers.h)
wraps generic module operations. This does not verify Sony VR function signatures
or make that entry point interchangeable with the selected SDK's loader ABI.

## 3. Relapse and KStuff establish different capabilities

[Relapse README at the pinned commit](https://github.com/ntfargo/Relapse-Exploit/blob/dd8e4e0914c5e9d066ee1f9b056be76cb992c8e0/README.md)
claims firmware 7.00–13.60 and an ELF loader on port 9021. It also documents
stability failures. These are upstream claims; no loader test was run here.

[KStuff README at the pinned commit](https://github.com/EchoStretch/kstuff-lite/blob/aaf921e608c9a3aaff753d92d4b3b16d2724b32d/README.md)
describes loader, crypto and kernel transition mechanisms. Its crypto handling
is not evidence of an application HMAC API for QuestBridge. Neither reviewed
README establishes a VR device replacement API or access to a game's compositor.
Exact installed builds, privileges and loader arguments remain unknown.

## 4. VR modules and the unavailable PS5VR source

GitHub's repository API again returned 404 for
[theghostonline/PS5VR](https://github.com/theghostonline/PS5VR).
A search index still describes a player using Hmd2, VrTracker2 and VrHand on
13.60, with firmware-specific export tables. This remains an **unverified lead**:
no live source, immutable commit, license file, signatures or export tables could
be audited. Do not promote cached descriptions to ABI evidence or 13.20 support.

Generic dynamic loading is demonstrated by the selected SDK's
[hello_dlfcn source](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/samples/hello_dlfcn/main.c).
That example loads a random-number module, not a VR runtime. It does not establish
that loading VR modules initializes absent hardware, exposes writable tracking,
or permits access to another process's stereo images.

The remaining distinction is fundamental: presenting a homebrew image to a real
PS VR2, replacing the physical display device, intercepting a game's frame
submission, and substituting tracking are separate research problems.

## 5. Gates before further Quest integration

| Gate | Evidence needed | Current state |
| --- | --- | --- |
| Build | Pinned SDK installation, compiler/linker versions, full log, inspected ELF and SHA256 | Passed for the stdout probe only; see build evidence |
| Startup | Exact loader ABI; reviewed entry path without automatic kernel writes | Default CRT unsuitable under project constraints |
| Firmware | Device-reported firmware and matching module build metadata | Owner target only; firmware gate stays disabled |
| Runtime | Verified names/signatures, required services and initialization result with absent PS VR2 | Unknown |
| Tracking/input | Observable native consumer and documented data layout, timing and validity | Unknown |
| Stereo | Verified game/compositor surface ownership, access and frame timing | Unknown |
| Reference title | Exact title/version plus initialization, pose, input and stereo evidence | Untested |

A native authenticated tracking receiver can be a later independent milestone,
but it will not clear the runtime, tracking injection or stereo gates. Review
socket wrappers, randomness, HMAC and timing before opening a service; preserve
loopback defaults and packet/session bounds. Do not use KStuff crypto internals
as a transport security implementation.

## Cloud experiment and next action

Initial shell requests failed to reach the configured proxy. Using the execution
tool's supported network permission restored access; the pinned SDK was cloned
and built. System package installation was read-only, so verified Debian package
archives were downloaded and extracted under `/workspace/questbridge-cloud`.
No proxy, routing or TLS bypass was used.

[Build evidence](../testing/PS5_BUILD.md) records the compiler, artifact hash,
commands and linked startup finding. The ELF contains the patch initializer call
before main; this is now an artifact observation as well as a source finding.
No payload was executed. GCC syntax checking also passed, separately from the
PS5 cross-compilation.

Continue with the native runtime consumer and detection investigation described in
PSVR2_DETECTION.md. The loader/startup findings remain a constraint before device
execution, not the main research objective. Keep all Linux outputs outside the
tracked Windows build/ directory. Quest development remains deferred.
