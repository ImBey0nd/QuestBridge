# Native PS VR2 interception method

Date: 2026-10-28. Decision: pursue a process-local replacement of the game's VR
consumer interfaces, with a virtual HMD output backend. This is a concrete
implementation target supported by public source, **not a demonstrated native
QuestBridge runtime**. No hardware ran during this investigation.

## Where to intercept

Redirect the imported VR functions in the application and its loaded middleware
(including engine PRXs), rather than attempting to make Quest USB traffic look
like PS VR2. Resolve and validate every provider and live relocation in its own
process. Public controller projects demonstrate the general import-redirection
approach; their success does not validate any VR ABI. See
[the pinned interposition sources](PS5_INTERPOSITION_SOURCES.json).

Install before the first VR initialization/availability query. A separate payload
resolving these functions does not affect the game. Cover later-loaded modules,
lazy binding, and function pointers obtained through runtime resolution as well
as static imports. Do not globally redirect unrelated Pad or VideoOut clients:
track the virtual headset, controller handles and HMD output ports explicitly.

The replacement must own the complete virtual session. In the owner's no-PS-VR2
configuration, forwarding initialization to Sony and only changing a pose result
still leaves absent hardware, tracker services and display queues. Implement the
measured application-facing contracts, backed by actual Quest tracking and a
working presentation backend; returning success from empty stubs is insufficient.

| Boundary | Replacement responsibility |
| --- | --- |
| Hmd2 Initialize/Open/device information/FOV/Close | Session state, valid handles, presence, optics, errors and teardown |
| Tracker2 QueryMemory/Initialize/RegisterDevice/GetResult | Device map, coordinate frames, time conversion, poses, validity and stale tracking |
| Pad Open/VR controller state/device information | Virtual Sense handles backed by Touch controls; ordinary DualSense calls retain their provider |
| Hmd2 BeginFrame/predicted time/SetParam/SetParamWithBuffer | Frame identity, scheduling, render pose, layers and submission correlation |
| VideoOut Open/RegisterBuffers2/configuration/retirement | Virtual bus-32 output, buffer generations, format/texture descriptors and completion semantics |
| Setup/calibration and gaze queries used by a title | Real capability negotiation and supported completion/cancellation behavior; Quest 2 has no eye tracking |

An initial supported subset must be explicit. Extra exports, setup policies and
engine paths prevent a claim that one fixed list already handles every title.

## The stereo boundary is registration plus reprojection

PS5VR's [app_vr.c at 209eb7f](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/app/src/app_vr.c)
provides concrete consumer evidence:

- Lines 825–846 open VideoOut bus 32 and register category 0x10 buffers.
- Each registered `data` points to a layer-set descriptor containing a pointer and
  count, not directly to linear image pixels. Its layer embeds AGC texture
  descriptors. This example repeats a side-by-side texture for the two eyes.
- Lines 771–798 call SetParam with buffer/frame IDs, per-eye regions, projection
  and render-pose/time metadata, then SubmitFlip. The engine intends to wait for
  its GPU fence, but its timeout fall-through is documented in the continuation.
- Lines 965–982 begin a frame, obtain predicted display time and request a pose
  for that time. BeginFrame alone is not its 60 Hz pacing mechanism.

An upstream [Horizon consumer report](https://github.com/boykopovar/AnyPS5/issues/7649#issuecomment-6274215852)
supplies a different path: two entries per registered buffer, 0x80-byte entries,
per-frame SetParam, and **no observed direct game flip to the HMD port**. It reports
calibration rendering with local emulation fakes, not native replacement on PS5.
Its tracker query is 0x30 bytes versus PS5VR's 0x20-byte query. Therefore neither
the player's one-layer layout nor its explicit flip is a universal contract.
The report is mutable testimony, not an immutable source commit or independently
reproduced measurement. A dated excerpt is recorded in the evidence JSON.

Use the following capture design:

1. On successful VR buffer registration, snapshot bounded descriptor metadata and
   associate it with process/module identity, port, set/index and a generation.
2. On either SetParam variant, associate frame ID, referenced buffer set, layer
   variant, eye regions, projection and render pose/time. Copy transient metadata
   while valid; a bare pointer retained after the call may become stale.
3. Establish which call or GPU command actually submits this frame. Observe both
   explicit flips and the runtime/graphics submission path. SetParam return is
   not a universal GPU-completion fence.
4. After a verified GPU dependency, copy/resolve/detile the source into an owned
   capture surface before the application may reuse or unregister it. Handle
   compression/auxiliary surfaces, MSAA and array layers according to the verified
   texture profile. Never treat an AGC descriptor as an RGBA pointer.
5. Encode and send the two views together with their actual render pose, FOV and
   timestamp. Release application buffers only after the required read/copy;
   maintain application-visible queue/flip events and backpressure separately
   from network/decoder completion. Drop stale output instead of growing a queue.

The buffer-set ID in SetParam is not assumed to equal a VideoOut buffer index for
every title. That association requires observation. PS5VR passes its index; it
does not prove the identity for other consumers.

## Tracking and feature constraints

The same source registers HMD type 0, left-controller type 2 and right type 1;
controller opens use Pad types 3 and 4. Buttons and connected state come from
Pad; controller poses come from Tracker2. Replacing only PadReadState cannot
provide 6DoF hands. The upstream consumer report describes additional state and
quality fields beyond pose values. PS5VR checks a different result word for its
own use. Neither check defines a complete universal tracking-validity record.

Size-query ABI matters: Hmd2 size/alignment queries return a pair of 64-bit
values, not a pointer or a single integer. The pinned AnyPS5 source documents
RAX:RDX. A generic integer-returning trampoline would corrupt this contract.
Preserve calling convention, output sizes, error behavior and thread safety per
verified function; do not generate callable shims merely from matched NIDs.

Quest and PS5 clocks need a measured mapping. Match each tracking query's desired
display time to a bounded prediction horizon. Carry the pose actually used for
rendering through video transport for Quest reprojection. Never substitute
packet arrival time for predicted display time, or label stale data tracked.

Expose missing gaze/headset vibration as unavailable where the consumer supports
that policy. Head direction is not eye tracking. Titles with compulsory missing
features may remain unsupported.

## Why other routes do not solve the current problem

- Physical USB emulation also requires DisplayPort negotiation/display delivery,
  device behavior, calibration and unresolved host authentication. A Quest
  application cannot turn its USB-C port into the PS VR2 DisplayPort sink.
- Sony tracker-daemon IPC replacement is a wider system boundary. The current
  public evidence does not specify its complete authenticated/lifecycle protocol.
- TV/Remote Play output is not guaranteed to contain both eye images and metadata.
  The social screen is not the proposed capture source.
- AnyPS5 OpenXR proposals run titles on a PC; useful consumer evidence, but they
  do not meet QuestBridge's native-console requirement.

## What is established, and what is still missing

Established from source: a viable general interposition technique, named VR
consumer boundaries, concrete registration/submission sequences, non-pixel VR
buffer descriptors, two submission variants and incompatible consumer assumptions.
Selected method: process-local VR interface substitution plus a virtual HMD
presentation/capture backend. A per-process adapter can be reused across titles;
firmware/module profiles and additional consumer features still need validation.

Not established: exact 13.20 callable contracts for all required functions,
verified live bindings in the owner's game process, complete GPU descriptor and
retirement handling, a native low-latency stereo encoder, or end-to-end performance.
No public encoder API was verified during this search; neither absence nor
availability of an accessible hardware encoder is inferred from search results.
Uncompressed 2000×2040×2 RGBA at 90 Hz is 23.5 Gbit/s before overhead, so ordinary
Wi-Fi cannot be the full-resolution raw-video fallback. Encoding is a separate
hard feasibility gate even after interception works.

The next console experiment, once separately permitted and correctly profiled,
is pass-through observation of registration, SetParam variants, actual GPU/flip
submission and retirement in an authorized native process. Its output should be
bounded metadata and result codes, not changed device answers. Before a title
exists, public-source research and offline discovery/contract tests continue;
no game installation or physical PS VR2 is a research prerequisite.

The offline coverage auditor accompanying this report identifies missing consumer
boundaries and conflicting import providers. Its output cannot authorize hooks
or enable firmware. Native interception and compatibility remain unverified.

## Reproduce the offline discovery

Run the existing `ps5-imports.mjs` scanner on an authorized local unencrypted ELF
snapshot as described in [PS5_IMPORTS.md](../testing/PS5_IMPORTS.md), then:

```sh
node tools/diagnostics/psvr2-boundaries.mjs /path/to/import-report.json
```

This also accepts the native scanner's raw version-1 JSON. It recomputes candidate
names from the pinned local catalog, distinguishes pointer relocations from other
relocations, and flags conflicting library/module declarations. Audit each engine
PRX too; a missing boundary in one image is not proof a title does not use it.
Even when every group is present, live binding, ABI and capture remain unverified.
The tool does not inspect processes or read memory at reported virtual addresses.

## Continuation: contract and feasibility decision, 2026-10-28

**Decision: keep process-local substitution as a research candidate; do not enable
or distribute a 13.20 replacement runtime. Neither successful substitution nor
its impossibility has been established.** The evidence refutes narrower shortcuts:
Pad-only interposition does not supply tracked hands; forwarding native device
initialization is not a demonstrated no-headset solution; registration pointers
are not pixel pointers; SetParam/SubmitFlip success is not GPU completion; a PC
emulator returning success is not native integration.

This continuation recovered the preceding eight-file milestone from the recorded
file changes and commands in the Codex chat **Find PSVR2 interception method**.
That chat records commit `fcae046`. The object was absent from this checkout and
GitHub returned 422 for that SHA; it has **not** been fetched as a Git object or
verified as an identical commit. The interface-catalog edits and the trailing
newline cleanup were also recovered. No skill installation was recovered: the
owner explicitly said to ignore it. New work is based on the recovered document,
not an assumed unpublished branch.

### 1. Hmd2 / Tracker2 / Pad contracts on 13.20

The [new source manifest](PSVR2_NATIVE_CONTRACT_SOURCES.json) pins 23 reviewed
public text files, their commit, Git blob and SHA-256. The cached text was checked
against the Git blob ID. These are primary sources for what their authors wrote,
not a primary measurement of the owner's firmware. No Sony library or game was
obtained. Source licenses were reviewed; all new implementation is original.

| Boundary | Source-level contract | Important gap before a native 13.20 binding |
| --- | --- | --- |
| Hmd2 lifecycle | PS5VR `app_vr.c:649–678`: 16 zeroed init bytes, four-slot Open, device-information query. AnyPS5 `Hmd2.hpp`: 28-byte info and 16-byte FOV; size queries return two 64-bit integers | App-category/user/message-bus policy, initialized errors, handles, full lifecycle and exact installed module builds |
| Tracker allocation/init | PS5VR `app_vr.c:681–724`: 32-byte memory query, 72-byte initialization and HMD registration. AnyPS5 API/Export agrees with these sizes | Options, work-memory ownership, daemon IPC and initialized behavior remain incomplete |
| Tracker results | PS5VR uses a 32-byte query. AnyPS5 explicitly declares legacy 32-byte **and** current 48-byte forms (`Export.cpp:27–41`); both put requested time at +16 | These can be versioned forms, not necessarily a contradiction. Selection by caller SDK/build and the complete output validity/prediction contract remain unknown on 13.20 |
| Controller buttons/connection | Source and consumer report agree on 104 bytes, buttons at +0 and connection at +64; Pad types 3/4 map to tracker types 2/1 | Poses still require Tracker2; device-info, haptic, history and disconnect semantics also matter |
| Controller timestamp | AnyPS5 `libScePad/Export.cpp:22` asserts **+72 (0x48)**; the upstream consumer report says **+96 (0x60)** | Real unresolved disagreement. Do not pick an offset by vote. That source's ReadState remains unimplemented (`:434–439`) |
| Submission | AnyPS5 declares 32-bit set ID, 64-bit frame ID, parameter pointer and fourth reserved slot for **both** variants | Calling registers alone do not resolve the layer union, all output/queue behavior or set-ID/index mapping |

The AnyPS5 header's 328/472-byte layer strides are documented in its debt record;
the layer types are only forward declarations. Compiling that header is not
confirmation that the layouts work. Its Initialize returns unsupported and its
Tracker2 GetResult returns not-initialized. PS5VR's firmware table labels 13.20
**library-checked**, with console tests on 12.40/13.60. Gyroaim reports a 13.20 Pad
hook on CFI-2015, not VR substitution on the owner's Fat model. None closes this gate.

### 2. Initialization without a physical PS VR2

PS5VR exits setup on a failed native Initialize/Open/tracker/reprojection stage;
it is a physical-headset consumer, not a substitute. AnyPS5 records checks on
`sceKernelGetAppInfo`, application fields and `sceMbusInit`; its tracker path opens
tracking/LED services and explicitly lacks daemon/work-memory completion. Therefore
a successful module load or initialization from a standalone payload cannot be
promoted to a working game session.

Reviewed alternative: [KytyPS5 Hmd2](https://github.com/KytyPS5/KytyPS5/blob/c528324d27bb6e6094d9efaf11fdbb88d27d8891/src/libs/libHmd2.cpp).
It supplies a virtual handle and generated display time inside its PC emulator.
Its SetParam checks parameters/logs and returns; this is not a native PS5 capture
backend. It does not resolve our service/queue problem. Its SetRenderConfig also
expresses the three consumed integer register slots differently (an aggregate and
option versus three integers in AnyPS5). Equal slot consumption does not establish
equal semantics. No emulator implementation or fake device answers were imported.

The candidate alternative to native forwarding is a complete process-local virtual
session, including the display wait/retirement contract. A failed shim that returns
zero must not be used to drive a title farther. Decisive evidence needed: an exact
13.20 module/caller profile plus a bounded before/after call trace with no headset,
in the relevant application context. No broad null-pointer probing was prepared.

### 3. Registration and both SetParam variants

Three records must remain distinct:

| Explicit offline profile | Header / per-layer span | What the prototype does |
| --- | --- | --- |
| `registration`: PS5VR one-layer registration | 16 / 304 (0x130) bytes | Copies opaque registered-layer metadata from a supplied snapshot |
| `plain`: AnyPS5 / PS5VR plain SetParam | 24 / 328 (0x148) bytes | Copies opaque layers and optional 128-byte option snapshot |
| `buffered`: AnyPS5 SetParamWithBuffer | 24 / 472 (0x1d8) bytes | Uses its own stride, never the plain stride |

`ps5/research/vr_snapshot.hpp` and `qb_vr_snapshot` implement this **offline** reader.
Every pointer is resolved only against supplied byte ranges, never dereferenced as
a live address. Overlap, integer overflow, missing ranges, truncation and excessive
allocation are rejected. The 16-layer/1-MiB limits are tool policy, not Sony limits.
They are manually selected **source profiles**, never selected automatically from
firmware or a familiar-looking header. No pixels or texture addresses are inferred.

The Horizon report's two 0x80-byte registration entries are another profile; the
tool deliberately does not interpret them as PS5VR's 0x130-byte layer. In particular,
successfully parsing a large contiguous snapshot with the wrong profile would not
detect all semantic mistakes. There is no signature that proves a profile is right.

No current code correlates a live game's registration, set ID, index and frame or
intercepts either variant. Need snapshots and call ordering from the actual consumer
to implement that adapter. Test fixtures contain invented metadata only. CLI usage
and limits are in [the reproducible experiments](../testing/PSVR2_NATIVE_EXPERIMENTS.md).

### 4. GPU synchronization, copy and release

New primary-source check: PS5VR's engine `eng_agc_runtime.c:2451–2512` polls its
own GPU marker, invalidating its cached line each time, with a 500-ms cap. After
the loop it invokes the VR submission callback and resets the expected marker
**without checking that the marker was reached**. Thus the earlier description
"after its GPU fence" describes the intended path, not a guaranteed success
condition. Exhausting a timeout is not permission to copy/free memory. We did not
execute or patch that upstream code.

`common/video/capture.hpp` provides a real bounded **CPU** RGBA crop/copy and an
explicitly offline observation-order checker. It checks frame **and generation**,
requires producer observation before copy, and distinguishes finishing capture's
source read from reusing encoder staging memory. A capture copy completing does
not retire the game's or compositor's other claims. Timeouts do not complete work.
It issues no GPU command, fence wait, cache operation or memory release.

The native adapter still needs all of: producer queue/fence identity and scope;
cache visibility; T# address/format/tiling/plane/array/sample/compression metadata;
resolve/detile/color conversion; an owned destination; copy completion; and the
application-visible wait/flip events. Completion events named GpuEnd/UserEvent in
a public header are candidates, not proof they cover a particular texture copy.
Polling an arbitrary word, CPU `memcpy` of tiled storage or reusing the source
while an asynchronous encoder reads it are rejected approaches.

A 2000×2040 pair is 32.64 MB RGBA. A full read plus write at 90 Hz is at least
5.8752 GB/s of memory traffic, before detiling, resolve, conversion and encoding.
This calculation is a budget, not a PS5 bandwidth measurement. Native GPU/zero-copy
encoding is the preferred experiment if a callable encoder can be established.

### 5. Real encoding experiment and alternatives

`encode-stereo.mjs` runs installed FFmpeg/libx264 with no B frames/lookahead,
observes **completed encoded packets before input EOF**, writes Annex-B H.264,
decodes all frames and checks first-frame left/right luma independently. Input is
explicitly synthetic. Only two frames may await output. This is real host codec
execution, not a simulation and not PS5/Quest integration.

The tool records startup and steady packet-observation latency separately; wall
throughput includes generation/startup/drain. Frame generation is separately timed.
These are not motion-to-photon, network, PS5 CPU or Quest hardware decode results.
`-framerate 90` supplies timestamps; it does **not** prove 90 frames/s processing.
FFmpeg's framecrc source confirms one record per completed AVPacket; tee allows
the same encoded packets to be inspected and saved. The installed Debian binary
and libx264 package are identified separately from reviewed FFmpeg source n7.1.5.

Results and exact commands/hashes are preserved in
[PSVR2_NATIVE_RESULTS.json](../testing/PSVR2_NATIVE_RESULTS.json).
For this server and these short patterns, 4000×2040 with 2 threads was about
25.1 frames/s with 94.9-ms p95 packet observation after startup; 4 threads gave
22.8 frames/s and 96.1 ms. Reducing to 2000×1020 gave about 91.1 frames/s, but
25.6-ms p95. A noisy 640×240 case gave 284.5 frames/s and 7.1-ms p95. These short
results neither isolate encoder time from the whole harness nor predict games;
they reject adopting this measured high-resolution host path as a 90-Hz solution.
The bars are unusually compressible, and the noisy test is low resolution.
The cloud cgroup grants two CPU equivalents (`cpu.max = 200000 100000`);
four encoder threads do not provide four CPU equivalents in this environment.
The x264 package's abbreviated upstream revision was resolved to immutable commit
`31e19f92f00c7003fa115047ce50978bc98c3a0d`; `common/base.c:675–685` confirms that
the zerolatency tune disables lookahead/B frames and enables sliced threads.
Package builds/patches are not claimed bit-identical to the reviewed sources.

Alternatives evaluated:

- More CPU threads did not rescue this run; no PS5 performance inference follows.
- Lower resolution improves host throughput but sacrifices detail and still needs
  target latency/decode/quality measurements. No resolution is silently negotiated.
- Native hardware encoding remains unverified. Public searches for PS5 encoders
  did not yield a callable, licensed 13.20 interface; Vita encoder stubs are a
  different platform. Search failure does not prove the hardware API is absent.
- Visor decodes **Quest-to-PS5 TV** video. It cannot supply a PS5 stereo encoder.
- External CPU encoding would first need stereo transfer: full RGBA is 23.5008
  Gbit/s, or 8.8128 Gbit/s for already converted YUV420p, at these dimensions/90 Hz,
  before overhead. Ordinary Wi-Fi/raw transport is not the proposed fallback.
- TV/Remote Play/HDMI capture remains unsuitable without evidence it contains both
  native eye images, their render pose and timing. No flat-screen substitute added.

### Decisive continuation boundary

All five native stages remain gated by specific evidence, rather than a generic
"needs hardware" claim. The [experiment matrix](../testing/PSVR2_NATIVE_EXPERIMENTS.md)
names each required observation, acceptance condition and alternative. Current
software does not contact a console or install hooks. The existing standalone
observer cannot produce VR/GPU observations and must not be injected into a game.
Further host refinements cannot determine these missing target contracts.
Proceed next with exact module/caller metadata and a separately reviewed read-only
observation method; do not activate firmware flags from these host results.
