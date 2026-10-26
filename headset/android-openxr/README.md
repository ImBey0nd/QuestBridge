# Android/OpenXR runtime probe scaffold
Status: source scaffold only. Not compiled, installed or tested on a headset.
This is a regular Android Activity with JNI. It initializes the Khronos loader, creates an OpenXR instance and asks for an HMD system, then destroys the instance. It does not create graphics bindings, sessions, tracking actions or video surfaces.
Published loader AAR 1.1.65 and prefab module were inspected; exact imported CMake target is checked against prefab metadata.

Provision JDK17, Gradle8.9, Android SDK35, NDK27.2.12479018 and CMake3.22.1. Set ANDROID_HOME and JAVA_HOME appropriately, then from this directory:
```text
gradle --version
gradle :app:assembleDebug
```
Expected output path if successful: app/build/outputs/apk/debug/app-debug.apk.
There is no Gradle wrapper or APK bundled. Dependency versions are pinned, but dependency checksums/lockfiles and build verification remain pending.
targetSdk32 is an initial diagnostic compatibility choice, not a store-ready configuration.
Next: verify compilation, runtime discovery and lifecycle on Quest 2; then implement session/action spaces and a diagnostic projection renderer using the official Khronos hello_xr reference. A successful probe is not a working headset client.

Local follow-up: JNI source syntax-check passed against Khronos 1.1.65 headers and desktop JNI headers. Android linking, Gradle build, APK packaging and runtime remain unverified. quest2-capabilities.json describes intended physical features, not completed client support.
