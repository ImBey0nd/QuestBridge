#!/usr/bin/env bash
# Build the runtime probe; this does not connect to or install on any device.
set -euo pipefail
: "${JAVA_HOME:?Set JAVA_HOME to a JDK 17 installation}"
: "${ANDROID_HOME:?Set ANDROID_HOME to the Android SDK directory}"
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root/headset/android-openxr"
exec ./gradlew --no-daemon --console=plain "$@" :app:assembleDebug :app:lintDebug
