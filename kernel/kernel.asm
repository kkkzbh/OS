

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
    out 0xa0, al    ; 向从片发送 EOI
    out 0x20, al    ; 向主片发送 EOI

    add esp, 4      ; 清理 error_code
    iret 

section .data 
    dd intr%1entry  ; 中断入口程序地址

%endmacro

VECTOR 0x00, ZERO 
VECTOR 0x01, ZERO 
VECTOR 0x02, ZERO 
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE  ; Double Fault - has error code
VECTOR 0x09, ZERO
VECTOR 0x0A, ERROR_CODE  ; Invalid TSS - has error code
VECTOR 0x0B, ERROR_CODE  ; Segment Not Present - has error code
VECTOR 0x0C, ERROR_CODE  ; Stack Segment Fault - has error code
VECTOR 0x0D, ERROR_CODE  ; General Protection Fault - has error code
VECTOR 0x0E, ERROR_CODE  ; Page Fault - has error code
VECTOR 0x0F, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE  ; Alignment Check - has error code
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ERROR_CODE  ; Control Protection Exception - has error code
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO
VECTOR 0x19, ZERO
VECTOR 0x1A, ZERO
VECTOR 0x1B, ZERO
VECTOR 0x1C, ZERO
VECTOR 0x1D, ERROR_CODE  ; VMM Communication Exception - has error code
VECTOR 0x1E, ERROR_CODE  ; Security Exception - has error code
VECTOR 0x1F, ZERO
VECTOR 0x20, ZERO  ; Timer interrupt (IRQ0)