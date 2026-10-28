# Test readiness research — 2026-10-27

## Deliverable status

Quest OpenXR discovery APK recovered from successful hosted build 40421881918,
artifact 12696579807, integrated source 41060fa6a217794ae92ca39308b10705453558c8.
Current Android source has no diff against that commit. Hosted lint report:
0 errors, 5 warnings. This session recovered the binary; it did not rebuild it or
run it on a headset. SHA256:
`18eef982913d77eabfe4e581e1548295d9c23df7db6f6416840af90188c110e1`.
Local file: artifacts/test-kit/questbridge-probe-debug.apk.

This app discovers an OpenXR runtime/HMD. It has no XR session, controller input,
tracking transport or stereo renderer. Its physical test is useful but does not
establish the intended QuestBridge end-to-end connection.

PS5 execution-ready artifact: **not delivered**. The previous SDK ELF remains
build-only. Missing old SDK installation/artifacts in this replacement workspace
is a reproducibility issue, not the substantive execution blocker.

## Sources checked again

- Relapse main remains `dd8e4e0914c5e9d066ee1f9b056be76cb992c8e0`:
  https://github.com/ntfargo/Relapse-Exploit/tree/dd8e4e0914c5e9d066ee1f9b056be76cb992c8e0
- SDK checkout remains `a7685ce8e97ce1f8a4f38253a41d54241c9124f1`:
  https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1
- PS5VR fork HEAD remains `209eb7f0160000507a0ca017842941b8e8e88f46`:
  https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/app/src/app_vr.c
- Reviewed loader reference:
  https://github.com/ps5-payload-dev/elfldr/blob/2a8da002e5292481ea964daa7342bbf4855f4c33/elfldr.c

These public sources were inspected, not executed. No console was contacted.

## Concrete loader finding

Relapse's src/kexp.js loads payloads/elfldr-ps5-1360.elf and constructs a 0x28-byte
argument block for its bootstrap shellcode. That block contains pipe descriptors,
allproc and loader image information. It is **not** evidence of the argument block
passed later to an uploaded application ELF.

The bundled loader SHA256 is
`de5dd480d12637527ba2d75e35de851adbb59c455293fa32588c492ef6b44b81`.
Offline inspection finds ELF headers at file offsets 0, 0x20000 and 0x40000, each
with 17 section headers. The dynamic symbol inventory does not expose the internal
elfldr_payload_args or elfldr_prepare_exec functions. A source-level name match
therefore has not established the bundled binary's startup ABI or source revision.
Those offsets describe this downloaded artifact only; they are not console offsets.

In the pinned reference loader, elfldr_payload_args writes a resolved getpid pointer
at offset 0 and a payloadout pointer at offset 0x28. elfldr_prepare_exec places the
argument block in RDI and prepares a return address. The SDK's payload.h instead
names that first field sys_dynlib_dlsym. Its syscall.c supports differing conventions;
a field name does not establish which function may safely be called.

The SDK's startup initializes kernel support and patches before running application
main (see PS5_FEASIBILITY.md). Replacing its main function does not remove that
startup. A custom startup must also justify relocation, stack alignment, imported
function resolution and termination. Simply removing a patch call or guessing a
resolver could create a broken artifact.

Next implementation prerequisite: identify the bundled application loader ABI and
its return/exit path, then implement and audit a minimal observation startup with
no SDK kernel/patch initialization. Build and inspect its complete call graph and
ELF imports before writing a specific physical procedure. The current evidence does
not establish this prerequisite. No repeat hardware inventory is needed.

## Native VR contract research

The unchanged PS5VR source confirms the existing lifecycle map:
Hmd2 initialize/open/device information; VrTracker2 memory/init/register/result;
reprojection initialization; owned eye-buffer registration and VideoOut flips.
Its frame path reads tracker output and submits its own buffers. It does not supply
external Quest poses to another game or capture that game's stereo buffers.

Thus the three independent open contracts remain detection/service initialization,
external tracking/input at game consumers, and stereo-buffer ownership/timing.
The next diagnostic should report actual module/return-code evidence, not fake
success for missing hardware. No verified game-runtime replacement was found in
these sources. No required reference game or physical PS VR2 was introduced.
