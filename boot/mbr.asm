

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

    mov byte [gs:BOOT_MARKER_BASE + 0x00], 'B'
    mov byte [gs:BOOT_MARKER_BASE + 0x01], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x02], 'O'
    mov byte [gs:BOOT_MARKER_BASE + 0x03], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x04], 'O'
    mov byte [gs:BOOT_MARKER_BASE + 0x05], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x06], 'T'
    mov byte [gs:BOOT_MARKER_BASE + 0x07], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x08], ':'
    mov byte [gs:BOOT_MARKER_BASE + 0x09], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x0A], 'M'
    mov byte [gs:BOOT_MARKER_BASE + 0x0B], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x0C], '1'
    mov byte [gs:BOOT_MARKER_BASE + 0x0D], BOOT_MARKER_ATTR
    mov byte [gs:BOOT_MARKER_BASE + 0x0E], ' '
    mov byte [gs:BOOT_MARKER_BASE + 0x0F], BOOT_MARKER_ATTR

    mov eax, LOADER_START_SECTOR
    mov bx, LOADER_BASE_ADDR
    mov cx, 4
    call rd_disk_m_16       ; 读取硬盘n个扇区 (这里是loader)

    jmp LOADER_BASE_ADDR + 0x300    ; loader加载完毕 跳转到loader执行

rd_disk_m_16: ; (eax: 扇区地址(LBA), cx: 扇区个数, bx: 目标内存地址)

    mov esi, eax         ; 备份数据
    mov di, cx 

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

.next_sector:
    call wait_disk_ready_16

    mov cx, 256          ; 一个扇区共 256 个字
    mov dx, 0x1f0        ; 指定端口 0x1f0

.go_on_read:
    in ax, dx            ; 读取数据
    mov [bx], ax         ; 将数据写入bx (函数参数，要写入的目标内存地址)
    add bx, 2            ; 移动bx到下一个字
    loop .go_on_read     ; 循环读取当前扇区

    inc esi
    dec di
    jnz .next_sector

    ret 

wait_disk_ready_16:
    push dx

    ; 读替代状态寄存器 4 次, 提供 ATA 规范要求的最小时序延迟
    mov dx, 0x3f6
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    mov dx, 0x1f7
.not_ready:
    in al, dx
    test al, 0x01        ; ERR
    jnz .disk_error
    and al, 0x88         ; al[3] 为 DRQ, al[7] 为 BSY
    cmp al, 0x08         ; DRQ=1 且 BSY=0
    jnz .not_ready

    pop dx
    ret

.disk_error:
    pop dx
    jmp $

    times 510 - ($ - $$) db 0
    db 0x55, 0xaa          ; MBR 引导扇区结束的魔数
