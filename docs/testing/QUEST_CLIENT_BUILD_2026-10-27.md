# Quest client build evidence — 2026-10-27

Current packaged APK source: `d7f2f629ad8bbb0304b4e0e52b168b437ede85ba`.
This final snapshot explicitly finishes NativeActivity on runtime-requested exit.
Earlier local snapshot dd3b912 is historical; its APK hash was
3305bcb559317b6a3acbbcf100e5e343c5135dd2c17d4bdd2bf8386187e5146d.
APK versionName `0.2.0-diagnostic`, versionCode 2, application ID
`org.questbridge.probe`, min SDK29, target/compile SDK35, ARM64 only.

Clean `tools/build/android.sh clean` completed successfully: 49 tasks executed,
assembleDebug and lintDebug passed. Lint: 0 errors, 8 warnings. These include fixed
orientation, ChromeOS ABI, backup rules, missing icon and hardcoded UI text; no
store certification is claimed. Debug APK v2 signature verified with apksigner.
Packaged manifest has Internet/OpenXR permissions and runtime queries, GLES3,
Quest2 metadata, private native ClientActivity and the exported launcher.

APK SHA256:
`9c57a6b0dd5abdecbca4c461c69f4dc897d90854f2b35e4c38fa747df64a255e`.
Local package: artifacts/questbridge-client-0.2.0-test-kit.zip. Includes APK, current
test procedure, Node receiver, GPL license, source archive from the exact commit
and checksums. The PowerShell helper is provided for the owner; it was not executed
on Windows in this Linux workspace. The packaged Node receiver is the same source
used by the successful interoperability test.

Toolchain: Java21.0.12.1 runtime/compiler modules, Gradle8.9, AGP8.7.3, command-line
tools19.0, SDK35, Build Tools34.0.0, NDK27.2.12479018, CMake3.22.1, Khronos loader1.1.65.
Command-line tools ZIP SHA256 matched the pinned official build setup. Gradle
wrapper distribution checksum verified. Java/Gradle used the configured session
proxy with TLS verification enabled. Debug signing keys differ between workspaces;
a previous APK installation may report a signing conflict.

Host evidence: Node27/27; native CTest6/6; JVM interop using the exact Android Java
transport against Node and C++ receivers, stale neutralization, receiver restart,
wrong MAC, nonce, zero session/sequence and mismatched ping echo passed. JVM fixture
poses are synthetic. Native eye projection/view math is host-tested, not GL-rendered
on a headset. EGL/OpenXR execution, pose behavior, device cadence, lifecycle and
Wi-Fi latency require the physical test described in QUEST_CLIENT.md.

No console, headset or game was contacted. Native PS5 runtime and stereo capture
remain unavailable; the previous SDK PS5 ELFs remain build-only. This package does
not contain a new executable PS5 observation artifact.

Fresh hosted validation of the final functional snapshot also passed:
- https://github.com/ImBey0nd/QuestBridge/actions/runs/40593745905
- https://github.com/ImBey0nd/QuestBridge/actions/runs/40593745916

Hosted APK and observer artifacts:
- https://github.com/ImBey0nd/QuestBridge/actions/runs/40593745916/artifacts/12775202859
- https://github.com/ImBey0nd/QuestBridge/actions/runs/40593745916/artifacts/12775512264

The hosted observer matches the local target hash exactly:
da11946e155bdbab9ad18d810d3577856b6efe811d56d7a79d21f46f24fec661.
The compiled observer also passed 19 isolated CPU-model cases with synthetic
syscalls; this does not constitute console execution. The earlier hosted APK at
source a683f29 had hash 2c9c2cad03bf752512eb1933c2e5460048005abe1a1860c40a02c5585cf48024;
APK hashes vary with source and environment/debug keys. Use the hash belonging to
the exact downloaded artifact, never a hash from a different build.
