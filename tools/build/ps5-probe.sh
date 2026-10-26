#!/usr/bin/env bash
# Compilation and offline inspection only. Never deploys or executes the ELF.
set -euo pipefail

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the installed pinned SDK}"
if [[ $# != 1 || "$1" != /* ]]; then
  echo "Usage: bash tools/build/ps5-probe.sh /absolute/output/directory" >&2
  exit 2
fi
source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
mkdir -p -- "$1"
output_root=$(cd -- "$1" && pwd)
case "$output_root/" in
  "$source_root/"*) echo "Use an output directory outside the checkout." >&2; exit 2 ;;
esac
for tool in make readelf sha256sum; do command -v "$tool" >/dev/null; done
test -f "$PS5_PAYLOAD_SDK/toolchain/prospero.mk"
echo "BUILD ONLY: default SDK startup writes kernel state before main. Do not deploy."
cp -- "$source_root/ps5/host/main.c" "$source_root/ps5/host/Makefile" "$output_root/"
"$PS5_PAYLOAD_SDK/bin/prospero-clang" --version > "$output_root/compiler.txt"
"$PS5_PAYLOAD_SDK/bin/prospero-lld" --version > "$output_root/linker.txt"
make -B -C "$output_root" 2>&1 | tee "$output_root/build.log"
readelf -h -d "$output_root/questbridge-probe.elf" > "$output_root/elf-inspection.txt"
(
  cd -- "$output_root"
  sha256sum main.c Makefile questbridge-probe.elf > SHA256SUMS
)
echo "Built and inspected: $output_root/questbridge-probe.elf (not executed)"
