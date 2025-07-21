

%include "boot.inc"

org 0x7c00
section mbr
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7c00
    mov ax, 0xb800
    mov gs, ax

    mov ax, 0x600           ; AH 功能号 0x06 | AL 上卷行数 为0表示全部
    mov bx, 0x700           ; AH 上卷行属性
    mov cx, 0               ; (0,0)
    mov dx, 0x184f          ; (80,25)

    int 0x10                ; 中断 0x10 实现清屏

    ; 输出  背景绿色 前景红色 跳动 内容为"KKKZBH"

    mov byte [gs:0x00], 'K'
    mov byte [gs:0x01], 0xA4    ; A表示绿色背景闪烁, 4表示前景色红色

    mov byte [gs:0x02], 'K'
    mov byte [gs:0x03], 0xA4

    mov byte [gs:0x04], 'K'
    mov byte [gs:0x05], 0xA4

    mov byte [gs:0x06], 'Z'
    mov byte [gs:0x07], 0xA4

    mov byte [gs:0x08], 'B'
    mov byte [gs:0x09], 0xA4

    mov byte [gs:0x0A], 'H'
    mov byte [gs:0x0B], 0xA4

    mov eax, LOADER_START_SECTOR
    mov bx, LOADER_BASE_ADDR
    mov cx, 4
    call rd_disk_m_16       ; 读取硬盘n个扇区 (这里是loader)

    jmp LOADER_BASE_ADDR + 0x300    ; loader加载完毕 跳转到loader执行

rd_disk_m_16: ; (eax: 扇区地址(LBA), cx: 扇区个数, bx: 目标内存地址)

    mov esi, eax         ; 备份数据
    mov di, cx 

; 读写硬盘

    mov dx, 0x1f2        ; 指定端口 0x1f2 1f是ATAO(Primary)通道
    mov al, cl           ; 写入要读取的扇区数
    out dx, al 

    mov eax, esi         ; 恢复eax
    mov dx, 0x1f3        ; 指定端口 0x1f3
    out dx, al           ; 写入要读取的扇区起始地址 低八位

    mov cl, 8
    shr eax, cl          ; 右移
    mov dx, 0x1f4
    out dx, al           ; 写入要读取的扇区起始地址 中八位

    shr eax, cl
    mov dx, 0x1f5
    out dx, al           ; 写入要读取的扇区起始地址 高八位

    shr eax, cl
    and al, 0x0f    
    or al, 0xe0          ; 1110 0000 位6表示LBA模式 5, 7符合规定固定为1 al[4] = 0 表示主盘
    mov dx, 0x1f6
    out dx, al           ; 写入要读取的扇区起始地址 最高四位和指定LBA模式

    mov dx, 0x1f7
    mov al, 0x20         ; 0x20 表示读取命令
    out dx, al           ; 写入读取命令 0010 0000

.not_ready:
    nop                  ; 空操作 增加延迟 (等待磁盘准备好)
    in al, dx            ; 写入后的读取状态寄存器
    and al, 0x88         ; al[4] 为1表示准备好了 al[7] 为1表示忙 取两位
    cmp al, 0x08         ; 表示读取命令执行完毕 al[4] = 1 and al[7] = 0
    jnz .not_ready       ; 如果非上述条件，继续循环

    mov ax, di           ; 一开始di存储要读取的扇区个数
    mov dx, 256 
    mul dx               ; dx|ax = 读取扇区个数 * 256字节 (不会溢出16位到dx，因为读取磁盘个数限制在255个以内)
    mov cx, ax           ; cx = ax = 读取的次数 (一次读取一个字(2字节) )
    mov dx, 0x1f0        ; 指定端口 0x1f0

.go_on_read:
    in ax, dx            ; 读取数据
    mov [bx], ax         ; 将数据写入bx (函数参数，要写入的目标内存地址)
    add bx, 2            ; 移动bx到下一个字
    loop .go_on_read     ; 循环读取 (loop每次执行默认自减 cx)

    ret 

    times 510 - ($ - $$) db 0
    db 0x55, 0xaa          ; MBR 引导扇区结束的魔数

