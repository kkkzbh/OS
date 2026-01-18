

[bits 32]
extern main
extern exit

global _start

section .text
_start:
    ; 确保两个寄存器和exec中load之后指定的寄存器一致
    push ebx    ; 压入argv
    push ecx    ; 压入argc
    call main

    ; 将main的返回值传递给exit, gcc用eax存储返回值(ABI)
    push eax
    call exit   ; noreturn