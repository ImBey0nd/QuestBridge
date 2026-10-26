# Research snapshot
Date: 2026-10-26. Public repository API metadata and pinned READMEs were inspected. JSON files in upstream/ record immutable SHAs, branches, upstream push dates, archive state and API license classification. No upstream payload was executed.
Confidence: HIGH=direct source/metadata; MEDIUM=upstream author's claim without local reproduction; UNKNOWN=no accessible confirming evidence. These are not QuestBridge hardware results.

| Repository/source | Commit, branch | Findings and confidence |
| --- | --- | --- |
| [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk) | a7685ce8e97ce1f8a4f38253a41d54241c9124f1, master | HIGH: dynamic-linking payload SDK, POSIX tools, Clang/lld and bash. Examples use toolchain/prospero.mk and C main. README lists elfldr/websrv among loaders; no blanket 13.20 certification. GPLv3+ except separately licensed BSD headers. Selected for a future minimal probe. |
| [PS5Dev/PS5SDK](https://github.com/PS5Dev/PS5SDK) | a2e03a2a0231a3a3397fa6cd087a01ca6d04f273, main | HIGH: beta, CMake/Ninja/Clang/lld, payload_main(struct payload_args*), no STL. Documented kernel helper targets 3.xx/4.xx, not 13.20. GPLv2. Much older push date than selected SDK; payload ABI must not be mixed between SDKs. |
| [Rufidj/PS5_DEV_HOMEBREW](https://github.com/Rufidj/PS5_DEV_HOMEBREW) | bcd3ec8f665a1ca7e546eca8168b89616c588816, main | MEDIUM: author reports user-space examples and dynamic module resolution tested on 13.00/13.20. Universal support is an author assertion, not demonstrated here. Make/shell examples; compiler requirements need source audit. MIT per metadata and README. |
| [PS5Dev/Byepervisor-Reborn](https://github.com/PS5Dev/Byepervisor-Reborn) | ea04f7132b837087f552c57261cb800481a3a910, main | MEDIUM: developer hypervisor exploit, lists 13.20/13.60 and untested 13.00, uses payload-dev SDK and elfldr/rest-resume behavior. Does not ship HEN patches. API license unclassified; per-file review required before reuse. Not needed for desktop work and not run. |
| [theghostonline/PS5VR](https://github.com/theghostonline/PS5VR) | unavailable | Live GitHub page and API return 404. Search index still describes a native video player and names three VR modules on 13.60. UNKNOWN: cannot audit source, signatures, build requirements, branch, license or hardware claims. Cached description is only a research lead. |
| [alvr-org/ALVR](https://github.com/alvr-org/ALVR) | adaa16bab3ef1a0c7aba4bebb5504c499c5a4425, master | HIGH: PC SteamVR streaming, lists Quest 2. Rust/native Android build tooling documented in linked build guide. MIT. Useful streaming reference; not native PS5 runtime integration. |
| [BnuuySolutions/PSVR2Toolkit](https://github.com/BnuuySolutions/PSVR2Toolkit) | a34894a61c3866a880e2c85c3cc99a4216ce86cb, main | HIGH: modification of official PC PS VR2 driver/app. API metadata says MIT; README also states noncommercial/eye-data restrictions, requiring legal/license clarification before reuse. Does not establish PS5 ABI. Windows driver ecosystem; full build requirements not audited. |
| [bps2414/visor-ps5](https://github.com/bps2414/visor-ps5) | b932610be7dfdd18b7d0ed3423b5ad59494a7582, main | HIGH: Quest-to-PS5 TV mirroring using ADB/scrcpy, FFmpeg and SDL2; opposite video direction. Ubuntu/WSL and pacbrew toolchain documented. GPLv3 with third-party notices. No proof of native PS5 VR capture. |
| [ntfargo/Relapse-Exploit](https://github.com/ntfargo/Relapse-Exploit) | dd8e4e0914c5e9d066ee1f9b056be76cb992c8e0, main | MEDIUM: README claims 7.00-13.60, ELF loader port 9021. MIT metadata. Does not certify arbitrary payloads or access to every process. |
| [EchoStretch/kstuff-lite](https://github.com/EchoStretch/kstuff-lite) | aaf921e608c9a3aaff753d92d4b3b16d2724b32d, main | HIGH: README discusses crypto/loader paths and optional diagnostics. API license unclassified. Exact per-firmware behavior and build requirements require source review. No documented VR tracking injection found in reviewed README; do not infer unrestricted privileges. |

All accessible repositories had archived=false at retrieval. A recent push does not guarantee maintenance or functionality. Metadata license labels are not a substitute for per-file review. Each source can be pinned using https://github.com/OWNER/REPO/tree/SHA from the table.

## VR module evidence
Search index of the unavailable PS5VR repository mentions libSceHmd2, libSceVrTracker2 and libSceVrHand. Because live source is unavailable, this does not verify callable exports or any function signature.
[Public developer directory listing](https://gist.github.com/psxdev/deb4de8d311aa9fda5dea4f39fad4e20) names libSceHmd2.sprx. This supports a filename, not a PS VR2 ABI or 13.20 behavior.
[Public AnyPS5 tester discussion](https://github.com/boykopovar/AnyPS5/discussions/4697) reports Hmd2 and VrTracker2 initialization/reprojection symbols. It is a third-party experiment in a different environment; no implementation from it has been imported.
libSceVrHand lacks an independently inspected live source/export artifact in this session. No verified signatures for any of the three are asserted. No synthetic structure is declared to match Sony.
Next evidence: owner-supplied module metadata (names/build IDs/export identifiers only, no proprietary binaries committed), process/loader logs, and non-invasive call traces from authorized hardware. A player for a real PS VR2 would not by itself demonstrate substituting that hardware.

## Android/OpenXR and video sources
- [Khronos OpenXR specification](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html): right-handed spaces, pose validity and predicted display timing. HIGH for conventions; our Android scaffold does not implement the frame loop.
- [Meta native samples](https://developers.meta.com/vr/documentation/native/android/mobile-openxr-sample/): platform sample reference, not evidence of QuestBridge operation.
- [Android NDK setup](https://developer.android.com/studio/projects/install-ndk): SDK manager installs native build tooling side by side.
- [Khronos loader package](https://repo.maven.apache.org/maven2/org/khronos/openxr/openxr_loader_for_android/): version 1.1.65 confirmed by downloaded Maven metadata. Loader AAR inspected for actual prefab target.
- [ALVR build guide](https://github.com/alvr-org/ALVR/wiki/Building-From-Source): streaming/Android build reference. Codec and reprojection integration remains unimplemented here.
No accessible evidence in this review demonstrates native PS5 VR compositor capture for an alternative headset. Hardware encoding availability, surface ownership and timing remain open questions.

## Environment and dependency inventory
Windows PowerShell, Git 2.56.0, CMake 4.4.4, Node 26.7.0 and JDK 26 detected. Python points to an unusable Windows Store alias. Visual Studio Build Tools lacks the C++ component. WSL is not installed. No Android SDK/NDK/Gradle/adb or PS5 SDK was detected in PATH or conventional checked locations.
Portable [w64devkit 2.10.1](https://github.com/skeeto/w64devkit/releases/tag/v2.10.1) downloaded into ignored .tools; compiler reports GCC 16.2.0. Third-party tool licenses remain in its distribution, not copied into project code.
Runtime desktop dependencies: Node built-ins only. Native common library: C++17 standard library only.
Planned Android dependencies: Gradle 8.9/JDK17, Android plugin 8.7.3, SDK35/min29, NDK27.2.12479018, CMake3.22.1, Khronos loader1.1.65. Scaffold only, build unresolved.
PS5 dependency: selected SDK at pinned SHA, bash/Clang18/lld18/make on POSIX. No Linux subsystem installation or exploit execution was performed.

## Feasibility assessment
Desktop protocol and reusable models are independently achievable and tested. A Quest OpenXR client is technically conventional but still needs its toolchain and device tests. A user-space PS5 payload appears plausible based on SDK evidence, without guaranteeing the requested loader/firmware combination.
Replacing headset detection, tracking/services and native stereo output remains unproven. There is no basis for a working-game promise or completion schedule until those interfaces are demonstrated.
