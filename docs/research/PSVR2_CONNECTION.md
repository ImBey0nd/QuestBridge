# PS VR2 connection investigation

Date: 2026-10-27. Scope: replace physical PS VR2 using Quest 2 while native games
execute on PS5. No console, USB device, game or firmware was exercised.

## Findings and architecture choice

A USB-C connector does not imply that PS VR2 video is an ordinary USB video stream.
The PC adapter exposes separate DisplayPort and USB connections, with power supplied
separately. Sony documents this arrangement in its
[PC preparation guide](https://www.playstation.com/en-sg/support/hardware/pc-prepare-ps-vr2/).
The [DP-AUX board](https://github.com/JX5S/DP_aux_emulator/tree/18a2240769b1a0a170c489f17c69eaa43abd6e2b)
addresses display negotiation for an actual PS VR2; it does not emulate a headset.
Its author records firmware-dependent EDID/DSC changes. Those changes demonstrate
why matching one descriptor is insufficient evidence of complete device behavior.

Two possible boundaries must be investigated separately:

1. Physical device emulation: USB functions, display sink/link negotiation, power,
   authentication and controller behavior. Sending a pose over Wi-Fi does not
   implement these. No working Quest-as-PS-VR2 physical adapter was verified.
2. Native runtime substitution: intercept the game's verified runtime consumers,
   supply tracking/input and acquire stereo surfaces before physical output.
   This is the preferred research direction for the software project, contingent
   on process access, service semantics and GPU ownership. It remains a hypothesis.

No public source inspected establishes a working replacement of all three native
contracts on firmware 13.20. Understanding a device wire format does not settle
these contracts. The objective remains general game support; no title is mandatory.

## Sources and independence

| Source | Pinned revision | Evidence and limits |
| --- | --- | --- |
| Nyabsi/monado-starvr | 27a1999f6d7fb4972570d63a06e437c93a61e49f | Inspected src/xrt/drivers/psvr2/psvr2_protocol.h and psvr2.c. Public host driver; not PS5 code. Header BSL-1.0. |
| BnuuySolutions/PSVR2Toolkit | a34894a61c3866a880e2c85c3cc99a4216ce86cb | PC driver augmentation; root MIT license. README has additional noncommercial-use language, which needs clarification before reuse. No implementation imported. |
| unterschall/psvr2-linux-adapter | f79efed21529dcc872e3e2a00944ac25f5872fff | GPL-2.0 project; protocol/hardware/reference docs corroborate Monado, but explicitly derive from it. Not an independent confirmation. No implementation imported. |
| JX5S/DP_aux_emulator | 18a2240769b1a0a170c489f17c69eaa43abd6e2b | Hardware source and historical display negotiation findings; actual PS VR2 on PC. |
| mtp-24/PS5VR | 209eb7f0160000507a0ca017842941b8e8e88f46 | Native homebrew consumer with physical PS VR2. Does not replace a game runtime. |

Monado protocol definitions were also cross-checked against the
[official generated source documentation](https://monado.pages.freedesktop.org/monado/psvr2__protocol_8h_source.html).
That page is mutable; the mirror commit above pins the inspected implementation.
Full source URLs use https://github.com/OWNER/REPO/blob/REVISION/PATH.

Sony publishes [headset open-source components](https://www.playstation.com/en-us/oss/ps-vr2/),
including Linux packages separated by headset firmware. This is headset software,
not the PS5 host runtime. The latest listed 6.10–7.00 package link timed out in the
browser fetch; its source was not inspected. No Sony header claim is represented
as independently verified from that package.

## Device streams

The inspected host driver separates SLAM, gaze, camera and status interfaces. These
are input streams from an actual headset, not native-game output buffers.

- SLAM record: 512 bytes, SLA marker, device microsecond timestamp, native position
  and quaternion plus unmodelled fields. The transfer allocation is 1024 bytes;
  allocation size must not be confused with the record format.
- Status: 32-byte header followed by 24-byte IMU records. Includes display receiver
  status, proximity, function button and IPD. Numeric status values are not a
  complete PS5 detection contract.
- Cameras: mode-dependent bulk payloads. A camera stream is not stereo game video.
- Gaze: separate stream and control commands. Quest 2 has no eye tracker; never
  advertise genuine eye tracking or silently synthesize valid gaze.
- LED/relocalizer/vendor streams: Monado dumps some opaque data. Their presence
  does not mean all device behavior is understood.

The Monado implementation remaps raw position and quaternion components, applies
SLAM correction, maintains quaternion sign continuity, estimates timing and uses
a sensor-to-head transform. Copying raw quaternion fields into an OpenXR head pose
would skip these steps. It also sets tracking flags in its host history without a
fully decoded wire validity field; QuestBridge's decoder preserves that unknown.
Do not copy PS VR2 sensor calibration into Quest 2 calibration.

## Authentication uncertainty

The Linux adapter documentation says its PC host does not exercise authentication.
This is specific to that path. The original researcher describes console peripheral
authentication in
[the headset research account](https://bnuuy.solutions/2026/02/01/ps-vr2-recovery-mode.html).
That account is evidence that authentication needs investigation, not proof of its
complete PS5 13.20 transcript. No authentication secret, bypass, exploit, firmware
modification or hardware claim was imported into QuestBridge.

Thus neither “no authentication exists” nor “device emulation is impossible” follows
from the reviewed evidence. Runtime substitution might avoid physical-device
acceptance only if the affected services and consumer boundary can be replaced.
That remains unverified.

## Native consumers and missing contracts

PS5VR calls Hmd2 initialize/open/device information, tracker memory/init/register/
result, reprojection setup and VideoOut buffer registration/submission. Its Sense
input uses libScePad calls separately from tracker poses. Its renderer owns the
buffers it submits. These are useful consumer facts, not a capture interface for
another game's GPU resources.

| Contract | Evidence needed before implementation |
| --- | --- |
| Device/service initialization | Actual module identity, process eligibility, absence behavior, initialization ownership and return semantics |
| Head/controller tracking | Callable ABI, data ownership, clock domain, prediction, origin/calibration and validity semantics |
| Input | Grip/aim distinction, button and analog semantics, connection state, haptic capability and focus handling |
| Stereo video | Image ownership/format, eye layout, GPU fences, presentation deadlines and an accessible encoder path |
| Process interposition | Which game's call sites can be redirected, lifetime of hooks and system-service interactions |

The existing interposition research supplies candidate input-hook mechanisms,
not proof that Hmd2/tracker/compositor contracts can be replaced globally.
A homebrew process successfully resolving a symbol cannot answer these questions
for all games. Return values must not be falsified to force initialization.

## Implemented diagnostic

`node tools/diagnostics/psvr2-slam.mjs RECORD.bin` decodes exactly one public-format
512-byte record from an already authorized local capture. It preserves native wire
coordinates, raw timestamp and unknown1. It rejects truncation, extra data, unknown
headers, nonfinite floats and nonunit rotations. It does not claim tracking validity,
align clocks, perform coordinate calibration, open USB, send packets or call Sony.
Output includes physical pose values: review it before sharing a capture.
No proprietary input is required or committed. Tests construct synthetic bytes.

This diagnostic supports future evidence comparison; it is not a runtime adapter.
Its strict header checks are the supported diagnostic profile, not a claim that all
future headset firmware must preserve those bytes.

## Next research dependencies

Inspect the published headset source's device control/authentication definitions
when accessible, keeping it separate from PC driver behavior. Resolve the actual
loader startup contract before creating an executable observation payload. Then
obtain native module/consumer observations sufficient to define one justified runtime
experiment. These are concrete unknowns, not a request for another hardware inventory.

The research is incomplete at the PS5 service/GPU boundary. More elapsed time alone
cannot convert those unknowns into verified ABIs or working universal compatibility.
