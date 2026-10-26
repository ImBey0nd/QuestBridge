# PS5 probe cloud compilation — 2026-10-26

**Result:** compilation and offline ELF inspection passed. No console execution.
The artifact is **build-only**: the default SDK CRT calls its kernel patch
initializer before main. See [the audit](../research/PS5_FEASIBILITY.md).

## Provenance and artifact

- SDK source: https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1
- Source remained unmodified; the build generated untracked SDK objects only.
- Compiler: Debian Clang 19.1.7 (3+b1); linker: Debian LLD 19.1.7.
- Target: `x86_64-sie-ps5`; project main compiled with `-Wall -Wextra -Werror`.
- Artifact: `/workspace/questbridge-cloud/ps5-probe/questbridge-probe.elf`, 110616 bytes.
- SHA256: `543ebfe9d71025cb0446abc6f70880a8cc061fed89036d4877ac112674fc0cae`.
- ELF64, little endian, x86-64, DYN/PIE; entry address `0x24` in this build.
- Dynamic dependencies: `libkernel_web.sprx`, `libSceLibcInternal.sprx`, `libSceNet.sprx`.
  These entries are linkage metadata, not evidence the modules resolved on hardware.
- Disassembly: `__crt_start` calls `__patch_init` before the call to `main`.
  Build-specific addresses are inspection evidence, not firmware offsets to reuse.

## Reproduce on a provisioned Linux host

Install ordinary LLVM/Clang/lld development tools, make and binutils. Build the
pinned open-source SDK into a separate directory, then run the compilation script:

```sh
# From a checkout of the pinned upstream SDK:
export LLVM_CONFIG=/absolute/path/to/llvm-config
make -j2 DESTDIR=/absolute/path/to/installed-sdk install

# From the QuestBridge checkout:
export PS5_PAYLOAD_SDK=/absolute/path/to/installed-sdk
bash tools/build/ps5-probe.sh /absolute/path/outside/checkout
```

The script copies the probe sources outside the checkout, forces a build, records
compiler/linker versions, inspects ELF headers/dependencies and hashes sources and
the artifact. It does not validate installation provenance: separately retain the
SDK source commit and confirm no tracked SDK source changes. It never invokes a
loader, deployment tool or the generated executable. Do not run upstream sample
`test`/`debug` targets, which can contact a console.

## This cloud environment

Initial shell network calls failed under default isolation. A supported network
permission enabled the configured proxy; clone and Debian downloads then worked.
Direct system apt installation failed on the read-only system directories.
Instead, Debian trixie package metadata was signature-verified by apt, required
package archives were downloaded with apt and extracted with dpkg-deb under
`/workspace/questbridge-cloud/llvm`. No system upgrade or root installation occurred.

Apt lists/cache/config were isolated under `/workspace/questbridge-cloud/apt`.
Downloaded packages include clang-19, lld-19 and llvm-19-dev and their missing
required dependencies. Existing system libraries also participate; this is not a
hermetic toolchain distribution. No third-party code or toolchain archive is
committed to QuestBridge.

For this prepared environment:

```sh
export LLVM_CONFIG=/workspace/questbridge-cloud/llvm/usr/lib/llvm-19/bin/llvm-config
export LD_LIBRARY_PATH=/workspace/questbridge-cloud/llvm/usr/lib/x86_64-linux-gnu
export PS5_PAYLOAD_SDK=/workspace/questbridge-cloud/ps5-sdk
bash tools/build/ps5-probe.sh /workspace/questbridge-cloud/ps5-probe
```

Full SDK build log: `/workspace/questbridge-cloud/ps5-evidence/sdk-build.log`.
Probe build, source hashes, compiler/linker versions and ELF inspection:
`/workspace/questbridge-cloud/ps5-probe/`.
Disassembly: `/workspace/questbridge-cloud/ps5-evidence/disassembly.txt`.
These cloud files may require rebuilding in a replacement environment; tracked
instructions and the artifact hash preserve the reviewable milestone.

## Other validation

- Node tests: 9/9 passed with the supported network permission for loopback UDP.
  The first default-isolation attempt failed in the two UDP integration files;
  the protocol test file passed. No code change was needed to obtain the passing run.
- Native C++ build and CTest: passed, 1/1 grouped test.
- Host C17 syntax check of main: passed; distinct from PS5 linking.
- Compatibility record validator: passed.

No firmware support flag or game compatibility record was enabled. No headset,
console, VR module or game was exercised. Next work is the loader/startup audit;
Quest development remains deferred.
