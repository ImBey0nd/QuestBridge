#!/usr/bin/env bash
# Build/inspect only. Default SDK startup patches kernel state; never deploy here.
set -euo pipefail
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the installed pinned SDK}"
: "${PS5_OPENSSL_ROOT:?Set PS5_OPENSSL_ROOT to the cross-compiled OpenSSL prefix}"
if [[ $# != 1 || "$1" != /* ]]; then
  echo "Usage: bash tools/build/ps5-native-host.sh /absolute/external/output" >&2
  exit 2
fi
source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
mkdir -p -- "$1"
output_root=$(cd -- "$1" && pwd)
case "$output_root/" in "$source_root/"*) echo "Use output outside the checkout." >&2; exit 2 ;; esac
for file in "$PS5_PAYLOAD_SDK/target/lib/libc++.a" "$PS5_PAYLOAD_SDK/target/lib/libc++abi.a" "$PS5_PAYLOAD_SDK/target/lib/libunwind.a" "$PS5_OPENSSL_ROOT/lib/libcrypto.a"; do test -f "$file"; done
compiler="$PS5_PAYLOAD_SDK/bin/prospero-clang++"
"$compiler" --version > "$output_root/compiler.txt"
echo "BUILD ONLY: SDK startup includes kernel patches. This script does not deploy or execute the ELF."
"$compiler" -DQB_PS5_PAYLOAD=1 -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
  -I"$source_root/common" -I"$source_root" -I"$PS5_OPENSSL_ROOT/include" \
  "$source_root/ps5/host/native/main.cpp" "$PS5_OPENSSL_ROOT/lib/libcrypto.a" \
  -o "$output_root/questbridge-native-host.elf" 2>&1 | tee "$output_root/build.log"
readelf -h -d "$output_root/questbridge-native-host.elf" > "$output_root/elf-inspection.txt"
(
  cd -- "$output_root"
  sha256sum questbridge-native-host.elf > SHA256SUMS
)
echo "Built and inspected: $output_root/questbridge-native-host.elf (not executed)"
