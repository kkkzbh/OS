

[bits 32]
extern main

global _start

section .text
_start:
    ; 确保两个寄存器和exec中load之后指定的寄存器一致
    push ebx    ; 压入argv
    push ecx    ; 压入argc
    call main