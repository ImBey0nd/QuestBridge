# Standalone PS5 observer candidate without SDK CRT

This is a newly implemented, bounded startup experiment, **not hardware tested**.
It is separate from the old SDK-linked stdout probe and native receiver, which
retain their existing build-only restrictions. This observer does not implement a
VR runtime, networking or game compatibility. No console deployment was performed.

## Purpose and supported contract

The selected profile is a loader that creates a **new standalone payload process**
and passes a mapped argument block in RDI whose first pointer is libkernel getpid.
It must never be used with a loader that injects into an existing game or system
process: successful completion invokes process exit. It is not a general loader
adapter, and recognition of firmware 13.20 does not select this profile.

Public reference:
https://github.com/ps5-payload-dev/elfldr/blob/2a8da002e5292481ea964daa7342bbf4855f4c33/elfldr.c
Its elfldr_payload_args resolves getpid NID HoLVWNanBBc and stores the pointer at
argument offset zero. Its spawn path starts a placeholder process. The separate
elfldr_exec path can target an existing process; this observer does not support it.

Offline follow-up on Relapse's bundled artifact (SHA256
`de5dd480d12637527ba2d75e35de851adbb59c455293fa32588c492ef6b44b81`):
its embedded ELF at container offset 0x40000 references that NID at image virtual
0x14e2f, calls a resolver at image 0x4080, and passes the result to the 8-byte target
write helper at 0x4790 with argument offset zero. Code at 0x3165 also prepares the
output-pointer field at argument offset 0x28. These are **artifact-specific offline
addresses**, not live PS5 addresses or reusable firmware offsets. They corroborate
the public convention without establishing byte-for-byte source/build provenance
or the currently installed loader's mode. No loader code is imported or executed.

## What the candidate does

The observer checks a strict 12-byte instruction profile at the supplied function:
`mov rax,20; mov r10,rcx; syscall`. Only after that check does it call getpid and
issue bounded stdout writes through the validated syscall instruction in the
existing libkernel stub. The stdout report is:

```text
QuestBridge observer v1: startup=custom; self_pid=positive; vr=unavailable
```

No PID, address, key or device identity is printed. Short stdout writes are handled
with forward progress and bounds checks; zero/error/overreported writes fail.
A new standalone process exits with status 0 on success or 1 on report failure.
If no usable stub is established, the standalone process traps instead of calling
an unverified syscall pointer or continuing into the placeholder application's
original main. A missing log or a trap is a failed experiment, never success.

This trust boundary assumes the loader supplies mapped readable arguments and
code; it is not a general defense against hostile unmapped pointers. The byte
profile is deliberately narrow and may reject legitimate alternative builds.
The syscall instruction offset 10 is derived from the checked bytes, corroborating
this SDK initializer rather than trusting a field name:
https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/crt/syscall.c
Syscall identifiers for exit(1), write(4), getpid(20) are public header facts:
https://github.com/PS5Dev/PS5SDK/blob/a2e03a2a0231a3a3397fa6cd087a01ca6d04f273/include/sys/syscall.h
These sources are not proof of target execution.

No SDK CRT, kernel access/patch initialization, imports, library loading, remote
process operations or firmware address tables are linked. The complete original
source is observer.c, observer.h and entry.S. `_start` aligns the stack, invokes the
bounded diagnostic, then its standalone termination routine. Returning into an
unknown placeholder context was rejected during development.

## Offline build and checks

```sh
QB_CLANG=/path/to/clang QB_LLD=/path/to/ld.lld \
  bash tools/build/ps5-observer.sh /absolute/output/outside/checkout
```

The NDK r27c Linux clang/lld can build the freestanding AMD64 ELF with target
x86_64-unknown-freebsd; no Android libraries are used. The script checks ELF64/DYN/
x86-64, entry inside an executable segment, 16 KiB load alignment, no interpreter,
no writable/executable load segment, no dynamic dependencies and no runtime
relocation tags. It retains compiler/linker versions, source/artifact hashes,
metadata inspection and complete text disassembly. Metadata alone is not a proof
of arbitrary program behavior; the source and generated call graph must also be
reviewed. The generated code's only external transfers use the validated getpid
entry and syscall instruction. No inline syscall instruction is emitted in text.

Host CTest uses inert callbacks: rejects null arguments and every byte mutation of
the accepted profile, checks getpid failure, short/error writes and standalone exit
status mapping. It does not execute FreeBSD syscalls on Linux or prove loader ABI.
The source tests and target ELF are separate artifacts. CI builds the candidate and
uploads its inspection files under questbridge-observer-candidate.

## Compiled machine-code emulation

The actual compiled ELF also passed 19 isolated Unicorn 2.1.4 CPU-emulation cases
with a synthetic libkernel-shaped getpid stub. The model intercepts every syscall;
only getpid, bounded stdout and standalone exit are permitted. Guest memory writes
must stay within its stack; argument memory and image code are read-only. Cases
cover complete/short/error/zero/overreported stdout, failed getpid, null arguments
and mutation of every accepted prefix byte. Unsupported input must trap without
making any syscall. Success must emit the exact report and exit 0; operational
failures exit 1. A return into the placeholder address is a failed test.

```sh
python3 -m venv /path/to/emulation-venv
/path/to/emulation-venv/bin/python -m pip install unicorn==2.1.4
/path/to/emulation-venv/bin/python tools/diagnostics/test-ps5-observer.py /path/to/questbridge-observer.elf
```

This executes guest machine instructions inside a CPU model, **not** FreeBSD/PS5
syscalls on the host. It validates the generated entry and helper register/stack
flow beyond inert callback tests. The synthetic syscall model is not a PS5 kernel,
loader, memory map, getpid implementation or physical result. CI retains its JSON
report with the exact ELF hash. Neither emulation nor matching hashes establish
actual target syscall permissions.

## Default socket-server evidence

The pinned socksrv.c selects elfldr_spawn for raw ELF input and pipes stdout through
the client socket by default. In the bundled image above, the raw ELF magic branch
at image 0xd65 calls the function at 0x31d0. That function invokes rfork_thread
through its dynamic import at 0x14a90, corroborating new-process creation. This
narrows the default server path; it is distinct from the bootstrap's injection
into an existing system process. These are offline artifact addresses only.

Source: https://github.com/ps5-payload-dev/elfldr/blob/2a8da002e5292481ea964daa7342bbf4855f4c33/socksrv.c
Do not apply the standalone exit policy to the bootstrap, an injection tool or
an existing-process target. No exact code-build provenance or installed-mode
observation is inferred merely from similar code.

## Current physical-test gate

Before any physical run, the loader mode must be established as a new standalone
process, with a known way to capture stdout and observe process completion. Do not
substitute an injection tool or an unknown existing-process target. The owner does
not need to repeat the general hardware inventory. This is the specific remaining
loader observation, not a request to install a game, buy PS VR2, update firmware,
run an exploit or change KStuff.

The repository's no-automatic-deployment rule remains in force. This document does
not issue a console upload command. Once the standalone mode and stdout/completion
path are resolved, the finite physical experiment is one candidate invocation,
recording its hash, exact stdout and completion. A failure requires review rather
than repeated invocation or removing instruction checks. It would establish only
user-space startup/stdout, not native receiver or PS VR2 functionality.
