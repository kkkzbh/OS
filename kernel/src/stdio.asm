

TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

global putchar

[bits 32]
section .text

putchar:
    pushad      ; 备份32位寄存器环境
    mov ax, SELECTOR_VIDEO
    mov gs, ax  ; 确保选择子选中视频段

    ; 获取光标位置(高八位)
    mov dx, 0x03D4  ; 寄存器索引 (类MAR)
    mov al, 0x0e
    out dx, al      ; 写入
    mov dx, 0x03D5  ; 寄存器数据 (类MDR)
    in al, dx       ; 读取高八位 (8位寄存器, 目标只能是al, 同理16只能是ax)
    mov ah, al

    ; 获取光标位置(低八位)
    mov dx, 0x03D4
    mov al, 0x0f 
    out dx, al 
    mov dx, 0x03D5 
    in al, dx 

    ; 将光标存入bx
    mov bx, ax     ; 习惯用bx作为基址寄存器 (光标坐标值)
    mov ecx, [esp + 36] ; pushad压入4*8=32字节 4字节返回地址 esp+36拿到参数

    cmp cl, 0xD
    jz .is_carriage_return ; CR 回车符
    cmp cl, 0xA 
    jz .is_line_feed       ; LF 换行符
    cmp cl, 0x8 
    jz .is_backspace       ; 退格键backspace
    jmp .put_other

.is_backspace:
    dec bx 
    shl bx, 1 ; 左移一位 (文本模式下一个字符2位)
    mov byte [gs:bx], 0x20 
    inc bx 
    mov byte [gs:bx], 0x07 ; 黑色背景(低4位), 浅灰色前景 (高4位)
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1
    mov [gs:bx], cl 
    inc bx 
    mov byte [gs:bx], 0x07 
    shr bx, 1 
    inc bx 
    cmp bx, 2000
    jl .set_cursor ; 如果没超出屏幕字符个数, 则去设置新光标值, 否则处理换行

.is_line_feed:              ; LF('\n')
.is_carriage_return:        ; CR('\r')

    xor dx, dx 
    mov ax, bx 
    mov si, 80              ; 25 x 80 (25行80列 文本布局)

    div si                  ; 获取行数

    sub bx, dx              ; dx存储余数 ax存储商

.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor          ; 代码上分两段逻辑，实际LF或CR都被当成CRLF的组合处理

; 滚屏，实现原理则仅仅只是平移行数据 (在一个数组中，空出的一行用空格填充)
.roll_screen: 
    cld 
    mov ecx, 960 ; 2000 - 80 = 1920个要搬运, 1920*2=3840 字节, 一次4字节, 960次
    mov esi, 0xC00B80A0 ; 第1行行首 src
    mov edi, 0xC00B8000 ; 第0行行首 dst 
    rep movsd

    mov ebx, 3840  ; 1920 * 2
    mov ecx, 80    ; 一次处理1个字符(2字节) 80次
.cls:
    mov word [gs:ebx], 0x0720  ; 0x0720 黑底白字的空格键 (20是空格 07是黑景白字)
    add ebx, 2
    loop .cls 
    mov bx, 1920        ; 将光标重置为最后一行的首字符

.set_cursor: ; (bx: 光标位置)
    mov dx, 0x03D4
    mov al, 0x0e 
    out dx, al 
    mov dx, 0x03D5
    mov al, bh  ; 高8位光标位置
    out dx, al 

    mov dx, 0x03D4
    mov al, 0x0F
    out dx, al 
    mov dx, 0x03D5 
    mov al, bl 
    out dx, al 
.put_char_done:
    popad 
    ret