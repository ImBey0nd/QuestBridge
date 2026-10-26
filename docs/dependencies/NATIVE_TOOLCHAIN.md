# Native receiver toolchain and cloud evidence

Date: 2026-10-26. No proprietary Sony SDK, firmware module, game binary or game asset
was used. Dependencies and build outputs remain outside the checkout.

| Dependency | Source/version | License | Evidence |
| --- | --- | --- | --- |
| PS5 payload SDK | a7685ce8e97ce1f8a4f38253a41d54241c9124f1 | GPL-3.0-or-later, separate BSD headers | Built from pinned public source |
| Compiler/linker | Debian LLVM/Clang/lld 19.1.7 | Apache-2.0 with LLVM exceptions and distribution notices | Workspace-local packages used |
| libc++, libc++abi, libunwind | LLVM 18.1.8 | Apache-2.0 with LLVM exceptions and per-file notices | SDK's documented configuration cross-compiled and installed |
| OpenSSL | 3.5.7, commit 8cf17aaeb4599f8af87fefd810b5b5fee90fe69e | Apache-2.0 | Host tests use installed 3.5.7; PS5 static libcrypto built from pinned source |

Sources:
- https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1
- https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/libcxx.sh
- https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8
- https://github.com/openssl/openssl/tree/8cf17aaeb4599f8af87fefd810b5b5fee90fe69e

LLVM source archive SHA256:
`0b58557a6d32ceee97c8d533a59b9212d87e0fc4d2833924eb6c611247db2f2a`.
The public release archive was fetched with normal proxy/TLS verification.
No upstream code was vendored into QuestBridge. Static-linked dependency licensing
and corresponding-source obligations still apply if distributing ELF artifacts;
retain the original notices and exact source revisions with such distributions.

## Prepared environment

```sh
export LLVM_CONFIG=/workspace/questbridge-cloud/llvm/usr/lib/llvm-19/bin/llvm-config
export LD_LIBRARY_PATH=/workspace/questbridge-cloud/llvm/usr/lib/x86_64-linux-gnu
export PS5_PAYLOAD_SDK=/workspace/questbridge-cloud/ps5-sdk
export PS5_OPENSSL_ROOT=/workspace/questbridge-cloud/ps5-crypto
```

Runtimes were configured from the LLVM source `runtimes/` directory using the pinned
SDK's prospero-cmake wrapper and its libcxx.sh options: static libc++, libc++abi and
libunwind, exceptions/RTTI/threads enabled, `-nostdlib++`, no shared runtimes,
LLVM unwinder, no compiler-rt and bare-metal libunwind. Install prefix is the SDK's
`target/`; install with `DESTDIR=/` to avoid the SDK shell's inherited DESTDIR
adding the prefix a second time. Full logs are under
`/workspace/questbridge-cloud/ps5-evidence/libcxx-{configure,build,install}.log`.
The source archive remains under `/workspace/questbridge-cloud/research/`.

OpenSSL source was checked out at the exact commit above, then configured in
`/workspace/questbridge-cloud/openssl-build` with SDK CC/AR/RANLIB:

```sh
perl /absolute/openssl/source/Configure BSD-x86_64 \
  no-shared no-asm no-tests no-apps no-dso no-module no-engine no-ui-console \
  --prefix=/absolute/ps5-crypto --openssldir=/absolute/ps5-crypto/config
make -j3
make install_sw
```

This creates a target libcrypto without dynamic provider-module loading. No target
OpenSSL executable was run; target entropy/provider behavior remains unverified.
Logs: `/workspace/questbridge-cloud/ps5-evidence/openssl-{configure,build,install}.log`.
Do not substitute host headers or host libcrypto when linking a PS5 payload.

## Recorded validation

- Native core tests cover HMAC/header integrity, truncation/corruption, wrong keys,
  malformed values, peer binding, replay/order, nonce retry, expiry/reconnect,
  local age gates, neutral fallback and concurrent snapshot consistency.
- Real Node/C++ UDP tests cover both directions of authentication, tracking,
  diagnostic video, PONG clocks, malformed/replay/endpoint rejection, oversized
  datagrams, receiver restart and invalid configuration before binding.
- Native parser and transport tests pass AddressSanitizer/UndefinedBehaviorSanitizer.
- PS5 receiver compiled with warnings as errors and was inspected as ELF64 x86-64 PIE.
- Import inventory finds no VR candidate imports; runtime/firmware gates stay disabled.
- SDK CRT initialization and patch routines remain linked. No PS5 ELF was executed.

The generated source/dependency/artifact hashes are in
`/workspace/questbridge-cloud/ps5-native-host/build-evidence.json` and SHA256SUMS.
These are cloud evidence, not device or game compatibility. The GitHub workflow is
configured locally; a successful remote CI run is not claimed.
