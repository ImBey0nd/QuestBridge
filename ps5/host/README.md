# PS5 host components

The authenticated C++ receiver lives in native/. See [native receiver usage/build evidence](../../docs/testing/NATIVE_RECEIVER.md). It is tested on a POSIX host and cross-compiled to PS5; target execution remains untested.

## Original stdout compilation probe

Cross-compiled into a PS5 ELF in the Linux cloud environment; never executed.
See [build evidence](../../docs/testing/PS5_BUILD.md).
This program prints a message; it is not a tracking receiver and opens no network service.

**Build-only:** the selected SDK's default CRT initializes kernel access and patches
credentials and syscall permissions before `main`. The earlier description of this
as a harmless user-space probe was incorrect. Do not deploy the current linked
payload under the project's no-kernel-write constraint. See
[the source audit](../../docs/research/PS5_FEASIBILITY.md).

Selected SDK: [ps5-payload-dev/sdk at a7685ce8e97ce1f8a4f38253a41d54241c9124f1](https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1).
Its installed `toolchain/prospero.mk` and C `main` convention match this scaffold;
its payload ABI must not be mixed with PS5SDK or the Rufidj examples.

After provisioning that SDK and its LLVM prerequisites, a **compilation-only** command is:

```sh
export PS5_PAYLOAD_SDK=/absolute/path/to/installed/ps5-payload-sdk
bash tools/build/ps5-probe.sh /absolute/output/outside/checkout
```

Expected output: `questbridge-probe.elf`, build log, compiler/linker versions,
ELF inspection and SHA256SUMS in the output directory. Preserve SDK source commit, installation
provenance, compiler/linker versions, full build log, ELF headers and SHA256.
Compilation does not verify the loader ABI, firmware behavior or console execution.
No deployment target is included.

The cloud SDK was built from the pinned source using workspace-local LLVM 19.1.7.
No proprietary Sony SDK was used. Reproduction and environment paths are in the
build evidence; the script expects an already provisioned SDK and LLVM tools.

Next: establish a loader-compatible startup path without automatic kernel writes,
then compile and inspect its linked entry path. Native authenticated networking now exists in native/ and is host-tested/cross-compiled.
VR module calls and game integration remain unimplemented.
