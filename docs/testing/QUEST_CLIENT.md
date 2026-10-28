# QuestBridge stereo/tracking client

Implemented 2026-10-27. APK 0.2.0-diagnostic, versionCode 2, ARM64, Android 29+.
Physical Quest execution is **not yet tested**. Build/lint and host interoperability
are distinct from headset results. No PS5 native-game integration is provided.

## What this version implements

A separate NativeActivity owns an OpenXR session with an EGL/GLES3 context and two
runtime-owned sRGB eye swapchains. The frame loop waits/begins/ends frames, locates
views at predicted display time and draws a fixed scene plus left/right grip markers.
READY begins the session; STOPPING ends it; exit/instance-loss ends the activity.
Window destruction tears down the graphics/session resources. Missing view validity
submits no projection layer. No frames or poses are fabricated when tracking is lost.

Head pose uses VIEW relative to LOCAL. Touch actions read grip poses, trigger,
squeeze, thumbstick, primary/secondary click/touch, left menu and stick click.
Absent/inactive actions remain neutral. Generic protocol button slots are not Sony
controller bitfields. Trigger-click and grip-click are not synthesized from analog
thresholds. Aim poses, haptics, eye tracking, video decoding and PS5 runtime support
are not implemented. Scene coordinates use LOCAL, not an assumed physical floor.

Native samples use the existing C++ canonical tracking encoder. The Activity passes
them to a single Java worker which authenticates protocol v1 with HMAC-SHA256,
pairs via a random nonce, accepts the receiver's nonzero session and checks reply
sequence and ping correlation. The socket connects to the explicitly entered IPv4
receiver. Unknown or stale sessions trigger re-pairing; no reply for two seconds
invalidates the current session. One pending sample replaces previous samples.
Samples older than 200 ms become identity poses with validity/input flags zero.
Android pause invalidates samples and prevents native callbacks from republishing
until resume. Destroy closes the transport without holding the Activity monitor
while NativeActivity waits for its native thread.

The pairing key and destination are handed to the native activity through a one-use
process-local object, not Intent extras or saved state. They are not saved in
preferences, files or logs. Process recreation requires entering them again.
Protocol v1 does not encrypt packets and carries no distinct per-device prediction
or capture timestamp. Its envelope uses the sender's monotonic clock; the logged
predicted XrTime stays in its OpenXR clock domain. No XrTime integer is reinterpreted
as a network timestamp. There is no measured device/network latency or cadence claim.

## Build and host checks

```sh
tools/build/android.sh
npm test
cmake -S . -B build-linux
cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
node tools/diagnostics/test-android-transport.mjs build-linux/qb_native_receiver
```

The Java interoperability test requires JDK 17+ (JAVA_HOME may select it). It compiles
the exact Android transport on the JVM and uses synthetic poses. It covers Node
receiver pairing, invalid samples, stale neutralization, restart/re-pair, wrong MAC,
wrong nonce, zero session/sequence, invalid ping echo and C++ receiver interoperation.
A host-native test verifies asymmetric projection corners, near/far clipping and
view/pose inverse math. These tests do not execute EGL or OpenXR on a headset.
The existing hosted workflow now runs the JVM checks after its native build.

## Owner test: local scene first

Install the **new version 0.2.0-diagnostic**, not the older discovery-only APK.
Keep its build commit/URL and SHA256. In Platform Tools:

```sh
adb install -r questbridge-client-debug.apk
adb logcat -v threadtime QuestBridge:I OpenXR-Loader:V '*:S' > questbridge-client-log.txt
```

On Windows PowerShell use `.\adb.exe` in place of `adb`. If signatures conflict,
retain the installation error; do not automatically uninstall or discard app data.
The debug key in this recreated cloud workspace may differ from the previous APK.

Open QuestBridge Diagnostics in Unknown Sources. Choose **Start stereo and tracking
diagnostic**. Expected if supported on this headset: a green cube and platform,
with blue left and orange right grip markers. Head movement should change your
view of the fixed scene; moving each controller should move its corresponding marker.
Keep the system's physical boundary active. Stop if the view is unstable or feels
uncomfortable. Save the exact error/log if it returns to the launcher.

Test controller sleep/wake, system-menu focus loss, visor suspend/resume, then exit
and reopen. Invalid or absent hands should disappear; logs should show corresponding
validity changes. Record whether the scene recovers without freezing or double vision.
The developer still needs these device observations before calling the client tested.

## Optional receiver test on your PC

This test needs no PS5, installed game, PS VR2 or proprietary files. Use Node 22+
and this repository on a PC sharing the Quest's private network. Generate a fresh
key locally, then start the desktop receiver on an explicitly selected bind address.
For PowerShell:

```powershell
$env:QB_KEY = node -e "process.stdout.write(require('node:crypto').randomBytes(32).toString('hex'))"
$env:QB_BIND = '0.0.0.0'
$env:QB_PORT = '42420'
node desktop/diagnostics/host.mjs
```

Enter the PC's local IPv4 and the generated key in the app, then choose **Start
with receiver**. The key can be displayed locally with `$env:QB_KEY`; never include
it in shared screenshots/logs. Allow UDP 42420 on the private network if the PC
firewall requires it. The receiver binds loopback unless explicitly configured;
no public internet access or port forwarding is required.

Expected if pairing works: receiver telemetry shows increasing tracking counts and
Quest logs say paired. Restart the receiver using the same key; the client should
re-pair after its reply timeout. Leave the app or suspend the headset and verify
fresh tracking does not continue as though valid while the app is paused. Capture
actual behavior rather than assuming the host tests reproduce Android scheduling.

This demonstrates a real Quest client sending tracking only after a physical run.
It does not establish external tracking accepted by PS5 games or native stereo capture.
