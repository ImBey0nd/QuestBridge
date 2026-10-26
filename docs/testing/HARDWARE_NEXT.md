# Hardware handoff: first missing evidence

The cloud milestones below are complete: read-only native import inventory,
authenticated C++ receiver, Node interoperability, freshness/neutral fallback,
native sanitizers, cross-compiled C++ runtimes/OpenSSL and PS5 receiver linking.
None establishes PS VR2 replacement or execution on the owner's PS5.

## First hardware-dependent step

Record the actual console/loading environment and obtain authorized local metadata.
Do not transmit the current receiver or old stdout ELF: both link the SDK CRT that
patches kernel state before main. Existing no-kernel-write/no-deployment constraints
remain in force. No new exploit execution or installation is requested here.

Use the console's own system-information screen and existing loader/homebrew logs:

| Item | Record | Current evidence |
| --- | --- | --- |
| Console | Exact model/revision | Owner reports PS5 Fat; exact revision not observed |
| Firmware | Full displayed version/build | Owner reports 13.20; full build not observed |
| Relapse/KStuff | Exact installed version/build or source commit | Unknown |
| ELF loader | Actual loader name, build/source, stdout behavior and existing logs | Unknown |
| VR modules | Names, sizes, build identifiers/hashes if available through existing authorized tooling | Public-source candidates only |
| Reference game | Region/title identifier and exact installed version | Resident Evil Village VR Mode requested; identifier unverified |

Share metadata/text results only. Keep proprietary module/game files and credentials
local. A hash of an encrypted file, decrypted ELF and live mapped image identifies
three different representations; always label which one it describes. No binary
upload is needed for these records. Do not run an unknown module merely to fill a row.

If an authorized unencrypted local ELF already exists, the offline inventory can
run where that file resides. [PS5_IMPORTS.md](PS5_IMPORTS.md) describes the command.
It changes no input bytes and produces metadata only. SELF/encrypted inputs are
unsupported; this project does not extract or decrypt them. Missing data should be
reported as unavailable, not replaced with assumptions.

## Recorded owner environment and upstream baseline

The owner reports a PS5 Fat running firmware 13.20 and prefers current upstream
Relapse/KStuff versions. Installed versions and any completed update remain unknown.
The full system build, exact revision and actual loader identity are still needed.

[PS5_ENVIRONMENT_BASELINE.json](../research/PS5_ENVIRONMENT_BASELINE.json) separates
this report from the public reference metadata checked on 2026-10-26. KStuff Lite's
latest published release is **1.11 Beta**, with its API-reported asset digest retained.
Relapse has no published releases; its current main source is pinned by commit.
These are reference identities, not an installation instruction or proof of console
compatibility. A bundled loader filename does not establish its startup contract.
No console update or hardware test has been performed by this project.

## Why the exact loader matters

The public loader and SDK show multiple first-argument conventions. In the pinned
[elfldr source](https://github.com/ps5-payload-dev/elfldr/blob/2a8da002e5292481ea964daa7342bbf4855f4c33/elfldr.c),
payload arguments start with a resolved getpid function pointer. The pinned
[SDK startup](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/syscall.c)
handles a resolver-pointer convention and a syscall-stub convention. A field name
alone therefore cannot justify calling that pointer as a generic dlsym callback in
a custom startup. No guessed replacement startup was added.

The next cloud task uses the recorded loader/build and metadata to select a justified
observation path. Only after its startup and call contract are reviewed should a
separate physical execution experiment be prepared. No firmware offsets are enabled
by recognizing a version string or matching a candidate function name.

## Later physical acceptance checks

Once a suitable observation artifact exists, retain its hash, loader/stdout logs,
process context, raw return codes and timestamps. A diagnostic receiver test would
need target entropy, bind/packet/reply behavior, reconnect, clock monotonicity,
threading and stale-sample neutralization. A VR experiment additionally needs actual
detection/lifecycle semantics, valid tracking/controller results and native stereo
buffer ownership. These are separate gates; passing UDP cannot clear the VR gates.

A connected physical PS VR2 baseline may help if one is already available, but no
purchase is assumed. The first environment/metadata step can proceed without it.
Quest app development remains deferred while the native PS5 boundary is unresolved.
