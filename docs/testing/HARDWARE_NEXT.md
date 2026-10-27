# Hardware handoff: first missing evidence

The cloud milestones below are complete: read-only native import inventory,
authenticated C++ receiver, Node interoperability, freshness/neutral fallback,
native sanitizers, cross-compiled C++ runtimes/OpenSSL and PS5 receiver linking.
None establishes PS VR2 replacement or execution on the owner's PS5.

## Confirmed environment and later hardware evidence

The owner confirmed the inventory on 2026-10-27. Do not ask them to fill it again.
Continue source research with these facts. The table distinguishes optional later
experiment details from the accepted development environment; it is not a checklist
the owner must complete before cloud work can continue.

For development-side intake, [PS5_ENVIRONMENT.md](PS5_ENVIRONMENT.md) provides a
bounded offline completeness checker and a blank JSON template. The owner's text
form remains sufficient. A complete declaration still requires source review and
does not enable execution or prove hardware compatibility.

For a concrete hardware experiment, identify only the specific extra observation
needed to justify that experiment. No new physical action is requested now.
Do not transmit the current receiver or old stdout ELF: both link the SDK CRT that
patches kernel state before main. Existing no-kernel-write/no-deployment constraints
remain in force. No new exploit execution or installation is requested here.

Use the console's own system-information screen and existing loader/homebrew logs:

| Item | Record | Current evidence |
| --- | --- | --- |
| Console | Exact revision only if needed by an experiment | Owner confirms PS5 Fat |
| Firmware | Full build only if needed for ABI verification | Owner confirms 13.20 |
| Relapse/KStuff | Pin source/build when reviewing startup | Owner confirms latest Relapse and Kstuff Lite 1.11 Beta |
| ELF loader | Actual loader name, build/source, stdout behavior and existing logs | Unknown |
| VR modules | Names, sizes, build identifiers/hashes if available through existing authorized tooling | Public-source candidates only |
| Games | Identity/version when a later test title is selected | No required reference title; Resident Evil Village not installed |
| Headset/controllers | Runtime build only when needed by a device test | Quest 2, latest software target, original Touch controllers |
| Physical PS VR2 | No purchase or connected baseline required | Owner has none |

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

The owner confirms PS5 Fat / firmware 13.20, Kstuff Lite 1.11 Beta, latest
Relapse, Quest 2 with original controllers, and no physical PS VR2. Target the latest
Quest 2 system software. Do not equate the word latest with an immutable source
commit; pin public sources during research and ask about a particular installed
build only if a specific experiment depends on it. Any native PS VR2 game is in
scope; installing Resident Evil Village is not a prerequisite.

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
