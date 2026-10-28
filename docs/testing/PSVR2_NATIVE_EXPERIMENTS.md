# Native VR substitution: reproducible experiments and evidence gates

Date: 2026-10-28. Target remains PS5 Fat / 13.20 / Kstuff Lite 1.11 Beta /
Relapse, Quest 2 and original Touch, with **no physical PS VR2**. Owner inventory
is accepted. No required reference title and no console action in this session.

## Reproduce the implemented host code

For a developer on Linux (the owner does not need to run these commands):

```sh
npm test
cmake -S . -B build-native-method -DCMAKE_BUILD_TYPE=Debug -DQB_TEST_HOST_ENCODER=ON
cmake --build build-native-method --parallel 2
ctest --test-dir build-native-method --output-on-failure
```

The optional encoder CTest requires FFmpeg/ffprobe with libx264 on Linux. Without
those tools omit `-DQB_TEST_HOST_ENCODER=ON`; the metadata/CPU tests still run.
The regular workflow now enables the small real codec test and sanitizes the
new C++ reader/copy/order test. That workflow has been edited, not run on GitHub.

The native metadata CLI takes binary snapshot bytes on stdin, an explicitly
selected public-source profile, snapshot base and parameter address as hexadecimal
numbers **without `0x`**:

```sh
build-native-method/qb_vr_snapshot plain 1000 1000 < snapshot.bin
build-native-method/qb_vr_snapshot buffered 1000 1000 < snapshot.bin
build-native-method/qb_vr_snapshot registration 1000 1000 < snapshot.bin
```

These examples are alternative interpretations, not three commands to try until
something parses. Select the profile from evidence of the actual record. A CLI
input represents one contiguous saved range; the C++ API supports up to 64
non-overlapping saved ranges. Total snapshot bound is 1 MiB and layer count bound
16. All pointed-to bytes must exist. This is **not a memory dumper**, hook or
Sony API caller. It emits counts/strides, with runtime/hardware flags false.

`registration` implements the PS5VR example only, not the Horizon report's 0x80
entries. Headers/layers/options are copied as opaque bytes. Parsing does not
verify firmware, record semantics, T# addresses or accessible pixels.

`copy_stereo` accepts owned, ordinary **linear CPU RGBA8** bytes with pitch and
explicit rectangles. It does not accept AGC descriptors, GPU addresses, compressed
or tiled surfaces, and performs no implicit detiling. Tests check exact distinct
eye pixels from padded rows. `CaptureOrder` is an offline validator of completion
observations, not an implementation of GPU waiting/copying/release.

Real host software codec experiments:

```sh
node tools/diagnostics/encode-stereo.mjs --eyeWidth=2000 --height=2040 --frames=48 --threads=2
node tools/diagnostics/encode-stereo.mjs --eyeWidth=2000 --height=2040 --frames=48 --threads=4
node tools/diagnostics/encode-stereo.mjs --eyeWidth=1000 --height=1020 --frames=60 --threads=2
node tools/diagnostics/encode-stereo.mjs --eyeWidth=320 --height=240 --frames=60 --threads=2 --pattern=noise
```

Run performance cases separately; concurrent workloads affect results. Input is
side-by-side synthetic YUV420p; `eyeWidth` is one eye, output width is double.
Bars have very low entropy; noise changes each frame at the small tested size.
At most two frames await completed encoder packets. A 60-second watchdog and
128-MiB packet-output bound stop failed/oversized experiments. Temporary H.264
outputs are retained under the output path printed in JSON. Decode checks all
frames and absence of B frames; the first frame additionally checks both eyes'
luma error. This is not a full image-quality or long-duration thermal test.

Latency starts immediately before writing a raw frame and ends when framecrc
reports the encoded AVPacket. It includes input queue/pipe/encoder/mux observation
and possible host scheduling. It excludes first-frame generation, GPU capture,
network, Quest decode, compositor/display and motion-to-photon. Generation of
subsequent frames can delay callbacks on the same Node event loop; it is timed
separately and no pure encoder-time claim is made. Startup is reported separately
from quantiles after the first four packets. No latency threshold is treated as
a hardware success flag.

## Decisive native experiments, in dependency order

These are specifications for a future reviewed observation method. **There is no
console-ready VR trace collector in this change.** Do not send the host binaries,
the old SDK-linked receiver, or the standalone observer into a game. No addresses,
patches, guessed calls or deployment command are provided.

| ID | Specific external evidence | Acceptance or rejection | Independent fallback if it fails |
| --- | --- | --- | --- |
| C1: contracts | Authorized private 13.20 metadata for Hmd2, Tracker2, Pad, VideoOut and graphics provider: whole-module hash/build fingerprint, caller SDK identity, names/NIDs and relevant import/export/relocation metadata | Bind an exact module/caller profile; unknown identity keeps every VR capability disabled. A matched NID alone fails | Review a second primary call site and exact local metadata; do not reuse 13.60 offsets |
| C2: no-headset init | Bounded original-call trace inside an eligible application: process category, call order, argument sizes, untouched-output canaries, raw results and timestamps for lifecycle; actual module identity attached | Identify first absence/process/service failure and valid rollback. A separate payload result, zero alone or printed state without query result fails | Full process-local virtual session after its contract is recovered; investigate daemon IPC only if its protocol is independently available |
| C3: stereo association | Successful registration snapshot and both submission variants, including port/set/index/generation, frame ID, layer type, submitted eye rectangles and resolved texture profile; original return codes preserved | A reproducible frame-to-two-eyes association without assuming set ID equals buffer index. Unsupported variants rejected | Observe runtime/graphics submissions and later-loaded engine modules; direct flip absence alone is not rejection |
| C4: GPU ownership | Producer dependency, cache/visibility rules, source texture layout, copy/resolve/detile command and completion, reuse/unregister timing and application-visible retirement | Compare two distinct known eye patterns; prove copy waits for writes and finishes before source reuse. Stress recycle, unregister, timeout and reordered completions. No stale generation may complete a new one | Owned linear staging after a verified GPU dependency; otherwise do not read. Mirroring only if independently shown to preserve both eye views and timing |
| C5: encoder | A documented/licensed native 13.20 encoder interface or buildable CPU encoder with measured PS5 resource budget; accepted input format, ownership, submit/receive/drain/error contract | Two-eye decode with matching frame IDs/render pose, bounded queue, startup/steady timings, losses and resource contention while a native title runs. An encoded test pattern alone fails native compatibility | Lower resolution/refresh only if explicitly negotiated and measured. Host x264 is a comparator, not a native backend. Raw Wi-Fi is not the full-size fallback |
| C6: whole path | Real Quest receive/decode/render plus a native PS5 VR consumer using the measured virtual session; headset absent throughout | Visible independent eyes, head/Touch motion, disconnect/reconnect and safe teardown; measured motion-to-photon; only actually tested titles/features listed | Isolate the failing layer and preserve prior observations; no universal compatibility claim from one title |

C1 and the design of C2 can progress without installing a particular game. Proving
game compatibility eventually needs an actual native consumer; any legally owned
title chosen for that experiment can supply it. We do not require Village or
buying PS VR2, and do not use someone else's PC-emulator report as console proof.

## What evidence to share

Prefer text reports produced by an already reviewed read-only tool. Keep original
proprietary modules, games and keys private. Needed report fields: firmware;
module/caller hashes and build identity; process category; missing/present physical
headset; attempt identifier; monotonic event times; original return codes; sizes;
bounded sanitized descriptor metadata; frame/generation correlation. GPU evidence
must identify the dependency it observed rather than only saying "fence done".

A report is still a supplied observation until its provenance is checked. Do not
upload memory dumps or publish proprietary binaries to resolve an offset dispute.
Until an appropriate collector or authorized primary record exists, the next
decisive native experiments cannot be performed from this server. More invented
fixtures cannot answer them. See [the Spanish owner note](PSVR2_OWNER_NEXT.md).
