# Native import interposition: investigation and first implementation

Date: 2026-10-26. Scope: finding the native game's VR consumer boundary before
writing a replacement runtime. Quest development remains deferred.

## New evidence

Public native-controller projects now provide concrete source evidence for
process-local import redirection. This advances the earlier finding that only
homebrew VR consumer code was available. It does not establish VR support.

| Source | Inspected revision | Relevant finding | Evidence limit |
| --- | --- | --- | --- |
| [PoorDS4 architecture](https://github.com/ItsBlurf/PoorDS4/blob/aa568a50ff9ef9797b08a61a6c248e1513333a31/docs/ARCHITECTURE.md) | aa568a50ff9ef9797b08a61a6c248e1513333a31 | Describes redirecting a running application's controller import slots after target/library/ABI checks | Author console evidence focuses on 11.60; exact layout recognition is distinct from successful input |
| [ps5-gyroaim](https://github.com/jmelovich/ps5-gyroaim/blob/3c62ed7041cdc6e113a1c6fcd523b3a1c146ea58/README.md) | 3c62ed7041cdc6e113a1c6fcd523b3a1c146ea58 | Author reports native-game import redirection on a CFI-2015 with firmware 13.20, kstuff-lite 1.11 and ShadowMount+ | Upstream claim, not reproduced; different console revision from the owner's original PS5 |
| [PuckbridgePS5](https://github.com/ThisIsAkill/PuckbridgePS5/blob/78460113ef7c48502c765a1f83ba34d53c977134/payload/game_hooks.c) | 78460113ef7c48502c765a1f83ba34d53c977134 | Source manages install/remove/feed lifecycle of in-game controller hooks | Reviewed as source only; no VR pose or compositor replacement demonstrated |

The [gyroaim inspection code](https://github.com/jmelovich/ps5-gyroaim/blob/3c62ed7041cdc6e113a1c6fcd523b3a1c146ea58/payload/hook.c)
walks loaded game objects, examines relocation destinations and matches their
current values against resolved controller functions. Its installation code uses
remote allocation, memory/protection changes and rollback. QuestBridge did not
execute or copy those operations. A live slot must be checked in its exact process;
a disk relocation alone is not a verified runtime hook location.

[Immutable source references](PS5_INTERPOSITION_SOURCES.json). PoorDS4 and gyroaim
publish GPL-3.0-or-later notices. No implementation was imported from them.

## Why this is enough to begin a scoped implementation

There is now evidence for the import boundary as an investigation path. There is
not enough evidence to implement Sony VR entry points, fabricate successful
headset initialization, write into a game or promise full PS VR2 replacement.

The first supported development step is a native, read-only metadata reader:
identify which imports have pointer relocations and attach source-backed candidate
VR names to their encoded identities. This can be built/tested without a console,
firmware offsets, proprietary binaries or guessed function signatures.

## Implemented

- `ps5/research/elf_imports.hpp`: original C++17 parser for bounded ELF64 metadata.
- `qb_import_scan`: host-native CLI that reads bytes from stdin and prints JSON.
- `tools/diagnostics/ps5-imports.mjs`: hashes the input snapshot, invokes the native
  reader and matches names against the existing public-source interface catalog.
- Native malformed-input and metadata tests, plus Node NID/classification tests.

The reader does not use section headers, so stripped inputs can be inspected.
It handles standard dynamic tags for SysV/GNU hashes, strings, symbols, RELA/PLT
relocations and DT_NEEDED. SCE module/library declarations preserve their separate
identifiers. PT_SCE_RELRO and object type constants follow the public SDK's
packaging source. The supported profile is an unencrypted ELF64 x86-64 image with
standard dynamic-table addresses mapped through load segments; other SCE layouts
are rejected rather than interpreted as this profile.

The [PS5VR metadata reader](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/tools/ps5_dynlib.py)
provides public evidence for the dynamic module/library fields and qualified NID
format. The [pinned SDK NID encoder](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/nid.c)
provides the name-hash convention. QuestBridge's implementation is original and
uses Node's built-in hash implementation instead of importing SHA1 code.

A matching NID identifies a candidate name, not a verified ABI. A matching library
name corroborates metadata only. It does not identify exact module code, firmware,
structure layouts, process permissions or whether the game actually calls it.

## Validation

- Native C++ build and both CTests passed, including stripped synthetic standard/SCE
  images, SysV/GNU hash tables, repeated/overlapping relocation ranges, all fixture
  truncations, bounds/overflow cases and 2000 deterministic malformed mutations.
- AddressSanitizer and UndefinedBehaviorSanitizer passed the native test corpus.
- Four NID vectors matched an offline host execution of the pinned public SDK encoder:
  Hmd2Initialize, VrTracker2QueryMemory, PadVrControllerReadState and Hmd2ReprojectionSetParam.
- End-to-end synthetic metadata recovered one candidate VR import and both of its
  relocation references. These bytes are invented test metadata, not console data.
- The real, previously cross-compiled QuestBridge stdout probe was inspected:
  SHA256 `543ebfe9d71025cb0446abc6f70880a8cc061fed89036d4877ac112674fc0cae`;
  its single referenced import is `puts`, with zero VR candidates. The reader did
  not execute this ELF or change its bytes.

The default execution sandbox initially timed out the Node subprocess invocation
and prevented LeakSanitizer from running under its tracing. Repeating under the
supported additional execution/network permission passed without disabling those
checks. The tools themselves make no network calls and require none on a normal
host. No code change was made to bypass the failed environmental checks.

## Remaining contracts before runtime development

1. Identify imports and module identities for the actual target title/build and
   firmware 13.20. Keep source binaries and private metadata outside the repository.
2. Verify detection, handles, process eligibility and lifecycle results. Public
   player calls cannot supply all game-process semantics.
3. Verify tracking result structures, validity flags, timestamps and Sense input.
   Controller import hooks do not themselves implement headset tracking.
4. Establish the native stereo submission boundary and GPU buffer ownership/timing.
   Input interposition does not make another game's surfaces accessible.
5. Verify process mapping, slot contents, conflicts, thread safety and recovery before
   considering any reversible runtime instrumentation.

Only import discovery/classification is implemented here. No injection, remote
syscalls, memory writes, DRM changes or console deployment were performed. The
existing firmware/runtime gates stay disabled. Next work uses actual authorized
local metadata or further public evidence to narrow the game's exact VR consumer
contract; runtime shims must wait for that contract rather than return false success.
