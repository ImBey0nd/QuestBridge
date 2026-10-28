// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Only field zero is read. The selected loader profile supplies a readable
// argument block and a readable/executable libkernel getpid function there.
// Other loader conventions are not supported by this observation artifact.
#if defined(_MSC_VER)
#define QB_OBSERVER_NORETURN __declspec(noreturn)
#elif defined(__cplusplus)
#define QB_OBSERVER_NORETURN [[noreturn]]
#else
#define QB_OBSERVER_NORETURN _Noreturn
#endif
struct qb_observer_args { const unsigned char* getpid_stub; };
int qb_observe(const struct qb_observer_args* args);
// Production definitions are in entry.S; host tests inject inert implementations.
long qb_observer_getpid(const unsigned char* stub);
long qb_observer_write(const unsigned char* syscall_instruction, int fd,
                       const char* bytes, unsigned long length);

// Standalone spawned-process profile only. This must never be used for a
// loader that injects into an existing application: it terminates its process.
QB_OBSERVER_NORETURN void qb_observer_finish(const struct qb_observer_args* args, int status);
QB_OBSERVER_NORETURN void qb_observer_exit(const unsigned char* syscall_instruction, int status);
