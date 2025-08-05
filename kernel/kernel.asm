

[bits 32]
%define ERROR_CODE nop  ; 主要用于统一对齐栈，不压入ERROR_CODE的中断程序就压入0
%define ZERO push 0

extern puts
extern idt_table ; C中注册的中断处理程序数组

global intr_entry_table
global intr_exit

section .data

intr_str db "interrupt occur!", 0xA, 0
intr_entry_table:

%macro VECTOR 2
section .text 
intr%1entry:
    %2
    push ds
    push es 
    push fs
    push gs
    pushad
    

    mov al, 0x20
    out 0xa0, al    ; 向从片发送 EOI
    out 0x20, al    ; 向主片发送 EOI

    push %1         ; 压入中断向量号
    call [idt_table + %1 * 4]
    jmp intr_exit

section .data 
    dd intr%1entry  ; 中断入口程序地址

%endmacro

section .text 
intr_exit:
    add esp, 4
    popad 
    pop gs 
    pop fs 
    pop es 
    pop ds 
    add esp, 4  ; error_code
    iretd

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
VECTOR 0x21, ZERO  ; 键盘中断对应的入口
VECTOR 0x22, ZERO  ; 级联用的
VECTOR 0x23, ZERO  ; 串口2对应的入口
VECTOR 0x24, ZERO  ; 串口1对应的入口
VECTOR 0x25, ZERO  ; 并口2对应的入口
VECTOR 0x26, ZERO  ; 软盘对应的入口
VECTOR 0x27, ZERO  ; 并口1对应的入口
VECTOR 0x28, ZERO  ; 实时时钟对应的入口
VECTOR 0x29, ZERO  ; 重定向
VECTOR 0x2a, ZERO  ; 保留
VECTOR 0x2b, ZERO  ; 保留
VECTOR 0x2c, ZERO  ; ps/2鼠标
VECTOR 0x2d, ZERO  ; fpu浮点单元异常
VECTOR 0x2e, ZERO  ; 硬盘
VECTOR 0x2f, ZERO  ; 保留