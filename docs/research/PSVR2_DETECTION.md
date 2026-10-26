# PS VR2 detection and initialization investigation

Date: 2026-10-26. Target: native PS5 games using Quest 2 and Touch controllers;
initial firmware 13.20. This milestone investigates the PS5 runtime boundary.
Quest app development remains deferred.

## Result

Follow-up: [native import interposition investigation](PS5_INTERPOSITION.md) found concrete public controller-hook source and started the read-only C++ import discovery component. The runtime and hardware limits below remain in force.

The missing PS5VR source was recovered through a public fork. GitHub identifies
`mtp-24/PS5VR` as a fork of `theghostonline/PS5VR`. Inspected revision:
`209eb7f0160000507a0ca017842941b8e8e88f46`. The original URL still returns 404.
This replaces the earlier search-index-only research limitation.

Public call sites now establish a concrete investigation map for Hmd2, VrTracker2,
Sense input and VR VideoOut. They do not establish a working replacement headset,
a callable firmware 13.20 ABI, or a way to redirect an existing native game's calls.

[Machine-readable map and source blob IDs](PSVR2_INTERFACE_MAP.json) preserve the
reviewed names and immutable URLs. No upstream implementation or firmware offset
table was copied into QuestBridge. Firmware integration remains disabled.

## Evidence and licensing

| Source | Revision | What was inspected | Evidence limit |
| --- | --- | --- | --- |
| [PS5VR fork](https://github.com/mtp-24/PS5VR/tree/209eb7f0160000507a0ca017842941b8e8e88f46) | 209eb7f0160000507a0ca017842941b8e8e88f46 | Native player call sites, module resolution, firmware claims and license | Source verified; reported console behavior not reproduced |
| [AnyPS5](https://github.com/boykopovar/AnyPS5/tree/bfbc55a45b18e2002695043b8531cc4b6a162406) | bfbc55a45b18e2002695043b8531cc4b6a162406 | Hmd2/Tracker2 implementation and explicit technical debt | PC implementation and author measurements; no native QuestBridge result |
| [DP-AUX emulator](https://github.com/JX5S/DP_aux_emulator/blob/18a2240769b1a0a170c489f17c69eaa43abd6e2b/README.md) | 18a2240769b1a0a170c489f17c69eaa43abd6e2b | Physical transport and historical display negotiation work | Uses a real PS VR2 on a PC; not headset substitution on PS5 |
| [psdaptwor](https://github.com/Swyter/psdaptwor/blob/852b5b38cde3b527954e1e739cdd69325010086b/README.md) | 852b5b38cde3b527954e1e739cdd69325010086b | Separation of DisplayPort, USB and power | Author labels this adapter experimental/nonfunctional at this revision |

PS5VR's notices state GPL-3.0-or-later for its app and separate terms for bundled
components. AnyPS5's root license is GPL-2.0; metadata alone does not establish
permission to combine every file with this project's GPL-3.0-or-later code.
This session reviews source and writes original diagnostic tooling, without
importing either implementation, generated tables, proprietary modules or assets.
The source references are technical evidence, not endorsement of upstream claims.

## What detection actually includes

[PS5VR's consumer code](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/app/src/app_vr.c)
provides the following observed call sequence. It is the sequence of a homebrew
player using physical PS VR2, not a verified universal game sequence:

1. Preload Hmd2 and VrTracker2; resolve required entry points.
2. Initialize Hmd2, query field of view, open a headset handle, and query device information.
3. Query tracker memory, initialize tracking and register the headset handle.
4. Allocate reprojection memory and initialize reprojection.
5. Create the player's own eye buffers, enable VR mode, open VR VideoOut, register
   buffers and configure output.
6. For frames, begin/predict display time, read tracking, submit reprojection
   parameters and flip the player's buffers.

This makes detection a lifecycle with multiple consumers. Reporting a headset as
present cannot alone provide tracking, display ownership, synchronization or input.
Source-level symbols identify investigation boundaries; they do not justify native
calls with guessed structures.

## Where headset substitution could fit

| Boundary | Existing public evidence | Still needed for QuestBridge |
| --- | --- | --- |
| Hmd2 initialization/device state | Player calls Initialize/Open/GetDeviceInformation; AnyPS5 has declarations and state handling | Target-firmware contract and real game's absence/presence behavior, including process eligibility |
| Tracking | Player calls VrTracker2GetResult for headset and controller handles | A verified way to supply external poses at that consumer boundary, with valid timestamps/status |
| Sense controller input | Player opens controllers via libScePad and reads VrController state | Touch-to-Sense mapping at a callable native boundary; pose and buttons are separate paths |
| Reprojection/VideoOut | Player registers and submits its own render buffers | A verified boundary for obtaining a native game's stereo buffers and retaining correct frame timing |
| Hand tracking | Separate VrHand initialization/start/result path | Optional later work; not required to first represent Touch controllers |
| Physical device | Public adapter projects expose USB, DisplayPort and power as separate channels | Device-side emulation and authentication evidence if choosing a physical-device approach |

The likely software investigation boundary is the game's runtime consumers, but
that is an engineering hypothesis. No native interposition mechanism was verified.
Resolving a function in our homebrew process does not redirect calls in another
process. A loader, privileged environment or dlsym success alone does not supply
that behavior. Hardware-device emulation is a separate, larger experiment; PC
adapter projects do not prove it can be implemented by sending tracking packets.

## What the recovered source changes for firmware 13.20

[PS5VR firmware documentation](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/docs/firmware-support.md)
lists 13.20 as checked against libraries, and 12.40/13.60 as console-tested.
These are upstream claims. Its table-selection header groups 13.20 with a 13.60
library table. QuestBridge has not inspected the owner's module builds or repeated
that comparison, so it does not enable 13.20 support or reuse those offsets.

The [resolver](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/app/src/app_vr.c)
tries named resolution, then an offset table selected by a string at a particular
readable location. This is more specific than the earlier assumption that generic
dlopen/dlsym is sufficient for all VR functions. Such a string is not a complete
module identity: unchanged text at the same location can survive other code
changes. Therefore we do not inherit the upstream claim that an anchor guarantees
the exact build, and do not adopt its fallback as a verified firmware gate.

## Cross-check against AnyPS5

The inspected [Hmd2 debt record](https://github.com/boykopovar/AnyPS5/blob/bfbc55a45b18e2002695043b8531cc4b6a162406/docs/dev/debt/libSceHmd2.md)
explicitly says there is no working PS VR2 runtime. Initialize returns unsupported;
tracking, rendering, gaze and most reprojection behavior remain absent. It also
identifies unmodelled application-info checks and message-bus initialization.
This suggests process context matters; a payload's result is not automatically a
game-process result. Its reported pre-initialization measurements on 13.60 do not
establish behavior on 13.20 or while initialized.

The [tracker debt record](https://github.com/boykopovar/AnyPS5/blob/bfbc55a45b18e2002695043b8531cc4b6a162406/docs/dev/debt/libSceVrTracker2.md)
says tracking never reaches initialized operation there and identifies missing
daemon IPC/work-memory handling and incomplete option/result semantics. Together,
these projects provide useful names and candidate structures, not a complete
substitute runtime. Their observations may share underlying research and should
not be counted as independent hardware confirmations.

The source-level disagreement/uncertainty is preserved: a typed declaration in
AnyPS5 or a byte-buffer layout in PS5VR is not an independently verified Sony ABI.
No native structures or hooks were generated from these references.

## Logging pitfalls and implemented offline diagnostic

The PS5VR consumer logs a frame as submitted periodically even if its reprojection
or flip calls failed. It also logs device status after a query without using that
query's return code to gate the log. Therefore neither a submission marker nor a
printed device-state value proves accepted output or valid detection.

Added `tools/diagnostics/ps5-vr-log.mjs` to interpret existing text logs offline:

- Records logged return codes/handles, first failure, missing stages and stage order.
- Separates attempts when initialization restarts; it cannot combine partial runs
  into a passing sequence.
- Does not treat unconditional frame markers as frame acceptance.
- Preserves raw device-status numbers without inventing enum meanings.
- Keeps declared firmware/origin separate from verification; even a declared console
  capture remains unverified by the analyzer.
- Bounds bytes, lines, line length and attempts; rejects malformed relevant status
  messages from a complete check sequence. It outputs no raw log lines or addresses.

[Usage and limits](../testing/PSVR2_LOGS.md). Tests use invented statuses only. No
console was contacted, no capture was uploaded and no proprietary binaries were read.

## Next measurable milestone

Identify the 13.20 device-detection contract at the runtime boundary, preserving:

1. Module identity and actual import/export metadata from an authorized local source,
   retained privately; do not commit proprietary modules.
2. A documented absence/presence result sequence with process context, exact builds,
   raw return codes and timestamps. Never start with a large set of guessed calls.
3. Which game consumer must be answered and whether a native process-local interception
   path exists. A homebrew initialization result does not clear this item.
4. A separately specified tracking-result/input and frame-submission contract after
   device detection; no false success responses to advance the game.

Cloud work can continue with source inspection and metadata/log tooling. The
remaining firmware/device behavior needs physical evidence. The existing default
SDK startup restriction remains in force for deployment; redesigning its startup
is not the main VR feasibility milestone. No automatic patches or console deployment
were performed, and Quest source remains unchanged.
