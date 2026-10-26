# Reproducible device evidence procedure
No console changes or deployment were performed during development. Never automatically load exploits, patch memory, or assume privileges from KStuff presence.

## Android
Provision JDK17, Gradle8.9, Android SDK35, NDK27.2.12479018 and CMake3.22.1. Build the probe as described in headset/android-openxr/README.md.
Record dependency resolution/build logs and APK SHA256. Compilation is separate from device testing.
On an authorized developer-mode headset, install via adb only when ready for device testing. Capture adb logcat with QuestBridge/Xr loader messages, headset OS version and runtime name.
Probe success means only loader initialization, instance creation and HMD system discovery. There is no tracking renderer yet.
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
