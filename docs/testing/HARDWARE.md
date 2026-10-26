# Reproducible device evidence procedure

Current next step: [hardware environment/metadata handoff](HARDWARE_NEXT.md). The native receiver now builds and passes host tests, but is not cleared for deployment with the default SDK CRT.
No console changes or deployment were performed during development. Never automatically load exploits, patch memory, or assume privileges from KStuff presence.

## Android
Provision JDK17, Gradle8.9, Android SDK35, NDK27.2.12479018 and CMake3.22.1. Build the probe as described in headset/android-openxr/README.md.
Record dependency resolution/build logs and APK SHA256. Compilation is separate from device testing.
On an authorized developer-mode headset, install via adb only when ready for device testing. Capture adb logcat with QuestBridge/Xr loader messages, headset OS version and runtime name.
Probe success means only loader initialization, instance creation and HMD system discovery. There is no tracking renderer yet.

The cloud build is now available; see [exact APK hash and build evidence](CLOUD_2026-10-26.md). Build via the checked-in wrapper/helper rather than a global Gradle installation. It targets SDK 35 and still supports installation from SDK 29. Signature verification is complete; installation and runtime operation remain untested.

For the owner-operated developer-mode Quest experiment, select the intended headset in ADB, verify the APK hash against the evidence and then use the following commands. If multiple devices are attached, add `-s SERIAL` to each command. Do not record device serials in public evidence.

```sh
adb install -r /path/to/questbridge-probe-debug.apk
# Start capture in one terminal before launching the app; stop with Ctrl+C.
adb logcat -v threadtime QuestBridge:I OpenXR-Loader:V '*:S' > questbridge-probe-logcat.txt
# In another terminal:
adb shell am start -W -n org.questbridge.probe/.MainActivity
```

Retain the install/start result, on-screen probe result, headset model/OS version and runtime name/error code. Repeat app launch after closing/reopening it and after headset sleep/wake; record crashes or lifecycle failures. An error is useful evidence and must not be replaced by a guessed success. The tag filter may miss vendor-specific loader diagnostics; widen capture locally if needed and remove unrelated private information before sharing logs. No commands in this procedure were run against hardware during the cloud session.

Future tracking test: verify stationary pose, +/- axes, controller actions, validity during occlusion, recenter and lifecycle pause/resume before networking.

## PS5 SDK compilation probe — device execution blocked
On a provisioned POSIX workstation install the pinned ps5-payload-dev SDK and prerequisites listed in research. Build ps5/host with make and PS5_PAYLOAD_SDK set.
Save compiler version, SDK commit, complete command, ELF SHA256 and loader version.
The pinned SDK's CRT calls `__patch_init` before `main`; it writes kernel credential and syscall-permission state. The current payload is therefore blocked from device execution under AGENTS.md, even though main only prints. See docs/research/PS5_FEASIBILITY.md for immutable source evidence.
Before designing a physical experiment, establish a loader-compatible startup path without these writes and inspect all startup code and constructors in the linked artifact. Removing a call alone does not establish loader compatibility or a safe runtime.
After that prerequisite, a separate owner-approved experiment must record firmware, hardware model, loader build/ABI, stdout routing, payload hash and complete return/failure logs. The expected main message is `QuestBridge SDK probe: main reached; no VR integration.` If stdout is not exposed by the loader, absence of a message does not prove success.
The main function does no VR module calls or network work; that statement does not characterize SDK startup. Stop on unexpected behavior; retain failure logs.
Native receiver networking/HMAC are now implemented and host-tested; PS5 target behavior and loader startup remain unverified.

## VR research evidence
Record date, firmware, module build IDs, accessible export names/identifiers, loader return codes and privileges. No undocumented call should be attempted until its ABI is justified. Keep proprietary module binaries local and out of Git.
A future compatibility test needs exact game region/title ID and version, required features, initialization logs, stereo capture evidence, pose response, controller tests and timing. Only populate database fields supported by observations.
No reference game was launched in this session.
