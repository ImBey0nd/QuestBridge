// SPDX-License-Identifier: GPL-3.0-or-later
// SysV AMD64 standalone-spawn entry; argument in RDI. Never inject into a game.
.text
.global _start
.hidden _start
.type _start,@function
_start:
 and $-16,%rsp
 sub $16,%rsp
 mov %rdi,(%rsp)
 call qb_observe
 mov %eax,%esi
 mov (%rsp),%rdi
 call qb_observer_finish
 ud2
.size _start,.-_start

.global qb_observer_getpid
.hidden qb_observer_getpid
.type qb_observer_getpid,@function
qb_observer_getpid:
 sub $8,%rsp
 call *%rdi
 add $8,%rsp
 ret
.size qb_observer_getpid,.-qb_observer_getpid

.global qb_observer_write
.hidden qb_observer_write
.type qb_observer_write,@function
qb_observer_write:
 mov %rdi,%r11
 mov %rsi,%rdi
 mov %rdx,%rsi
 mov %rcx,%rdx
 mov $4,%rax
 sub $8,%rsp
 call *%r11
 add $8,%rsp
 ret
.size qb_observer_write,.-qb_observer_write
.global qb_observer_exit
.hidden qb_observer_exit
.type qb_observer_exit,@function
qb_observer_exit:
 mov %rdi,%r11
 mov %esi,%edi
 mov $1,%rax
 sub $8,%rsp
 call *%r11
 ud2
.size qb_observer_exit,.-qb_observer_exit
.section .note.GNU-stack,"",@progbits
