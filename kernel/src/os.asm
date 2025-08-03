

global set_cursor

[bits 32]
section .text

set_cursor:  ; bx: u16
    push ebp
    mov ebp, esp
    push ebx
    mov bx, [ebp + 8]

    ; 设置高8位
    mov dx, 0x03D4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03D5
    mov al, bh  ; 高8位光标位置
    out dx, al

    ; 设置低8位
    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x03D5
    mov al, bl
    out dx, al

    pop ebx
    pop ebp
    ret