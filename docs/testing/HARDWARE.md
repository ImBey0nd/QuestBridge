# Reproducible device evidence procedure
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

## PS5 SDK smoke test
On a provisioned POSIX workstation install the pinned ps5-payload-dev SDK and prerequisites listed in research. Build ps5/host with make and PS5_PAYLOAD_SDK set.
Save compiler version, SDK commit, complete command, ELF SHA256 and loader version.
Console deployment is a separate owner-operated experiment. Do not transmit the probe until the exact firmware, loader and user-space behavior are confirmed and the owner has approved the physical experiment.
Capture stdout/loader logs containing the exact QuestBridge SDK probe message, firmware version and hardware model. If stdout is not exposed by the loader, absence of a message does not prove success.
Probe does no VR module calls, process writes or network work. Stop on unexpected behavior; retain failure logs.
Next native receiver requires reviewed SDK networking and native HMAC implementation first.

## VR research evidence
Record date, firmware, module build IDs, accessible export names/identifiers, loader return codes and privileges. No undocumented call should be attempted until its ABI is justified. Keep proprietary module binaries local and out of Git.
A future compatibility test needs exact game region/title ID and version, required features, initialization logs, stereo capture evidence, pose response, controller tests and timing. Only populate database fields supported by observations.
No reference game was launched in this session.
