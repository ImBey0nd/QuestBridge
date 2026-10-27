#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Execute the compiled observer in an isolated CPU model with synthetic syscalls.

Requires unicorn==2.1.4. No PS5 code/modules, native target execution, or networking.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_64, UC_HOOK_INSN, UC_HOOK_MEM_WRITE
from unicorn import UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC
from unicorn.x86_const import (UC_X86_REG_RIP, UC_X86_REG_RSP, UC_X86_REG_RDI,
                              UC_X86_REG_RSI, UC_X86_REG_RDX, UC_X86_REG_RAX,
                              UC_X86_REG_EFLAGS, UC_X86_INS_SYSCALL)

if not __debug__:
    raise RuntimeError('Observer emulation requires Python assertions enabled')

PREFIX = bytes.fromhex('48c7c0140000004989ca0f05')
REPORT = b'QuestBridge observer v1: startup=custom; self_pid=positive; vr=unavailable\n'
BASE, ARGUMENT, STUB, STACK = 0x1000000, 0x2000000, 0x3000000, 0x4000000
STACK_SIZE = 0x10000
MASK = (1 << 64) - 1


def run_image(data, name, mode='full', pid=123, mutation=None, null_argument=False):
    machine = Uc(UC_ARCH_X86, UC_MODE_64)
    entry, phoff = struct.unpack_from('<QQ', data, 24)
    entsize, count = struct.unpack_from('<HH', data, 54)
    for i in range(count):
        kind, flags, offset, address, _, filesz, memsz, _ = struct.unpack_from('<IIQQQQQQ', data, phoff + i * entsize)
        if kind != 1:
            continue
        permissions = ((UC_PROT_READ if flags & 4 else 0) | (UC_PROT_WRITE if flags & 2 else 0)
                       | (UC_PROT_EXEC if flags & 1 else 0))
        assert address % 0x1000 == 0 and offset + filesz <= len(data)
        machine.mem_map(BASE + address, (memsz + 0xfff) & ~0xfff, permissions)
        machine.mem_write(BASE + address, data[offset:offset + filesz])
    machine.mem_map(ARGUMENT, 0x1000, UC_PROT_READ)
    machine.mem_write(ARGUMENT, struct.pack('<Q', STUB))
    machine.mem_map(STUB, 0x1000, UC_PROT_READ | UC_PROT_EXEC)
    prefix = bytearray(PREFIX)
    if mutation is not None:
        prefix[mutation] ^= 0xff
    # Synthetic libkernel-shaped stub; syscall hooks below implement all behavior.
    machine.mem_write(STUB, bytes(prefix) + b'\xc3')
    machine.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    machine.mem_write(STACK + STACK_SIZE - 8, struct.pack('<Q', 0xdead0000))
    machine.reg_write(UC_X86_REG_RSP, STACK + STACK_SIZE - 8)
    machine.reg_write(UC_X86_REG_RDI, 0 if null_argument else ARGUMENT)
    output = bytearray()
    calls = []
    result = {'exit': None, 'trap': False}

    def memory_write(uc, access, address, size, value, context):
        assert STACK <= address and address + size <= STACK + STACK_SIZE, 'Unexpected guest memory write'

    def syscall(uc, context):
        assert uc.reg_read(UC_X86_REG_RIP) == STUB + 10, 'Syscall outside validated synthetic libkernel stub'
        number = uc.reg_read(UC_X86_REG_RAX)
        calls.append(number)
        if number == 20:
            returned = pid
        elif number == 4:
            assert uc.reg_read(UC_X86_REG_RDI) == 1
            length = uc.reg_read(UC_X86_REG_RDX)
            assert 0 < length <= len(REPORT)
            payload = bytes(uc.mem_read(uc.reg_read(UC_X86_REG_RSI), length))
            assert payload == REPORT[len(output):], 'Unexpected report or incorrect partial-write pointer'
            if mode == 'error':
                returned = -1
            elif mode == 'zero':
                returned = 0
            elif mode == 'overreport':
                returned = length + 1
            else:
                returned = min(3, length) if mode == 'short' else length
                output.extend(payload[:returned])
        elif number == 1:
            result['exit'] = uc.reg_read(UC_X86_REG_RDI)
            uc.emu_stop()
            return
        else:
            raise AssertionError(f'Unexpected syscall {number}')
        uc.reg_write(UC_X86_REG_RAX, returned & MASK)
        uc.reg_write(UC_X86_REG_EFLAGS, uc.reg_read(UC_X86_REG_EFLAGS) & ~1)

    machine.hook_add(UC_HOOK_MEM_WRITE, memory_write)
    machine.hook_add(UC_HOOK_INSN, syscall, None, 1, 0, UC_X86_INS_SYSCALL)
    try:
        machine.emu_start(BASE + entry, 0, timeout=1_000_000, count=10000)
    except UcError:
        pc = machine.reg_read(UC_X86_REG_RIP)
        assert bytes(machine.mem_read(pc, 2)) == b'\x0f\x0b', 'Unexpected CPU emulation error'
        result['trap'] = True
    assert bytes(machine.mem_read(ARGUMENT, 8)) == struct.pack('<Q', STUB)
    if mutation is not None or null_argument:
        assert result == {'exit': None, 'trap': True} and not calls and not output
    elif pid <= 0:
        assert calls == [20, 1] and result['exit'] == 1 and not output
    elif mode in ('error', 'zero', 'overreport'):
        assert calls == [20, 4, 1] and result['exit'] == 1 and not output
    else:
        assert calls[0] == 20 and calls[-1] == 1 and result['exit'] == 0 and not result['trap']
        assert output == REPORT
    return {'case': name, 'syscalls': calls, **result}


def main():
    if len(sys.argv) != 2:
        raise SystemExit('Usage: python test-ps5-observer.py COMPILED.elf')
    data = Path(sys.argv[1]).read_bytes()
    assert len(data) < 1024 * 1024 and data[:6] == b'\x7fELF\x02\x01'
    cases = [run_image(data, 'full stdout'), run_image(data, 'short stdout', mode='short')]
    cases += [run_image(data, mode + ' stdout', mode=mode) for mode in ('error', 'zero', 'overreport')]
    cases += [run_image(data, 'getpid failure', pid=-1), run_image(data, 'null argument', null_argument=True)]
    cases += [run_image(data, f'unsupported prefix byte {i}', mutation=i) for i in range(len(PREFIX))]
    print(json.dumps({'evidence_kind': 'compiled_elf_cpu_emulation', 'backend': 'Unicorn synthetic syscall model',
                      'elf_sha256': hashlib.sha256(data).hexdigest(), 'cases_passed': len(cases),
                      'hardware_tested': False, 'cases': cases}, indent=2))


if __name__ == '__main__':
    main()
