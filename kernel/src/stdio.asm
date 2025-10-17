

TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

global putchar
global puts
global puthex
global clear

section .data
buf dq 0  ; 8个字节的缓冲 用于数字到字符的转换

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

puts:
    push ebx 
    push ecx 
    xor ecx, ecx 
    mov ebx, [esp + 12]
.goon:
    mov cl, [ebx]
    cmp cl, 0
    je .str_over
    push ecx
    call putchar 
    add esp, 4  ; 回收压栈的空间
    inc ebx 
    jmp .goon 
.str_over:
    pop ecx
    pop ebx 
    ret

puthex:
    pushad  ; 压8个寄存器 4*8=32字节
    mov ebp, esp 
    mov eax, [ebp + 4 * 9]
    mov edx, eax 
    mov edi, 7  ; 在buf中的初始偏移量
    mov ecx, 8  ; 32位数字 16进制对应8位
    mov ebx, buf

.16based_4bits:
    and edx, 0x0000000F
    cmp edx, 9
    jg .is_A2F
    add edx, '0' ; 低8位不会溢出
    jmp .store
.is_A2F:    
    sub edx, 10 
    add edx, 'A'
.store:
    mov [ebx + edi], dl
    dec edi 
    shr eax, 4
    mov edx, eax 
    loop .16based_4bits

.ready_to_print:
    inc edi
.skip_prefix_0:
    cmp edi, 8
    je .full0
.go_on_skip:
    mov cl, [buf + edi]
    inc edi 
    cmp cl, '0'
    je .skip_prefix_0
    dec edi 
    jmp .put_each_num
.full0:
    mov cl, '0'
.put_each_num:
    push ecx 
    call putchar 
    add esp, 4
    inc edi 
    mov cl, [buf + edi]
    cmp edi, 8
    jl .put_each_num
    popad 
    ret

clear:
    pushad
    ; 由于用户程序的cpl为3，显存段的 dpl为0
    ; 故用于显存段的选择子gs在低于自己特权的环境中为0
    ; 导致用户程序再次进入中断时，gs为0
    ; 故直接在puts中每次都为gs赋值
    mov ax, SELECTOR_VIDEO  ; 不能把立即数直接送入gs，要由ax中转
    mov gs, ax 

    mov ebx, 0
    mov ecx, 80 * 25

.cls:
    mov word [gs:ebx], 0x0720  ; 0x0720 黑底白字的空格键 (20是空格 07是黑景白字)
    add ebx, 2 
    loop .cls 
    mov ebx, 0

.set_cursor     ; 直接把set_cursor搬过来用 (cv上方代码)
    mov dx, 0x03D4  ; 索引寄存器
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

    popad 
    ret

