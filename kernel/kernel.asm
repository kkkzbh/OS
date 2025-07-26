

[bits 32]
%define ERROR_CODE nop  ; 主要用于统一对齐栈，不压入ERROR_CODE的中断程序就压入0
%define ZERO push 0

extern puts

global intr_entry_table

section .data

intr_str db "interrupt occur!", 0xA, 0
intr_entry_table:

%macro VECTOR 2
section .text 
intr%1entry:
    %2
    push intr_str
    call puts 
    add esp, 4

    mov al, 0x20
    out 0xA0, al    ; 向从片发送 EOI
    out 0x20, al    ; 向主片发送 EOI

    add esp, 4      ; 清理 error_code
    iret 

section .data 
    dd intr%1entry  ; 中断入口程序地址

%endmacro

VECTOR 0x00, ZERO 
VECTOR 0X01, ZERO 
VECTOR 0x02, ZERO 