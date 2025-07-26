

%include "boot.inc"

org LOADER_BASE_ADDR
section loader
    ; jmp loader_start

    ; 构建gdt及其内部的描述符
    GDT_BASE:           dd 0x00000000
                        dd 0x00000000

    CODE_DESC:          dd 0x0000FFFF
                        dd DESC_CODE_HIGH4

    DATA_STACK_DESC:    dd 0x0000FFFF
                        dd DESC_DATA_HIGH4

    VIDEO_DESC:         dd 0x80000007   ; limit = (0xBFFFF - 0xB8000) / 4K = 0x7
                        dd DESC_VIDEO_HIGH4 ;

    GDT_SIZE equ $ - GDT_BASE
    GDT_LIMIT equ GDT_SIZE - 1
    times 60 dq 0  ; 此处预留60个描述符的空位(slot)

    SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
    SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

    total_mem_bytes dd 0    ; (0xB00 = 0x900 + 0x200)

    ; gdt指针 前2字节为gdt界限 后4字节为gdt起始地址 共48位

    gdt_ptr:
        dw GDT_LIMIT
        dd GDT_BASE

    ards_buf times 244 db 0  ; 用于存储ARDS结构体数组
    ards_nr dw 0              ; ARDS个数    结构体数组大小为了对齐256字节


loader_start: ; (地址 0xC00 文件地址0x300)


    ; int 15h eax = 0xE820, edx = 534D4150("SMAP")  获取内存布局

    xor ebx, ebx
    mov edx, 0x534D4150
    mov di, ards_buf
.e820_mem_get_loop:
    mov eax, 0xE820
    mov ecx, 20
    int 0x15
    jc .e820_failed_so_try_e801
    add di, cx 
    inc word [ards_nr]
    cmp ebx, 0
    jne .e820_mem_get_loop
    
    ; 找出ards结构中最大的内存块
    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx        ; 用于存最大的内存容量
.find_max_mem_area:
    ; 断言最高的内存块一定是可被使用的    
    mov eax, [ebx]
    add eax, [ebx + 8]  ; 获取内存块高度 (暗示最大容量)
    add ebx, 20
    cmp edx, eax 
    jge .next_ards
    mov edx, eax 
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

    ; int 15h ax = E801h 0-16MB 16MB-4GB的内存检测
.e820_failed_so_try_e801:
    mov ax, 0xE801
    int 0x15
    jc .e801_failed_so_try88        ; 若E801失败 尝试0x88方法

    ; 先算出低15MB的内存
    mov cx, 0x400      ; 乘数 低15MB(ISA设备扩展占用1MB) 以KB为单位存储
    mul cx             ; 结果为 dx:ax
    shl edx, 16        ; 为拼接乘法结果做准备
    and eax, 0x0000FFFF; 仅保留有乘法结果的ax(低16位) 确保结果正确
    or edx, eax 
    add edx, 0x100000   ; 加上ISA设备扩展占用的1MB
    mov esi, edx        ; 存入esi中
    ; 将16MBa以上内存转为字节单位 (64KB单位)
    xor eax, eax 
    mov ax, bx 
    mov ecx, 0x10000
    mul ecx         ; 由于在结果一定4G以内 不会溢出 eax会存有最终结果
    add esi, eax 
    mov edx, esi    ; 最终完整内存大小存入 edx 中
    jmp .mem_get_ok

    ; int 15h ax = 0x88 获取内存大小 只能获取1MB-64MB以内的内存
.e801_failed_so_try88:
    mov ax, 0x88
    int 0x15        ; KB为单位 ax存取个数
    jc .error_hlt
    and eax, 0x0000FFFF ; 只有低16位(ax)存有正确结果, 于是确保

    mov cx, 0x400
    mul cx
    shl edx, 16
    or edx, eax 
    add edx, 0x100000 ; 加上1MB (0x88只能获取1MB以上内存)  

.mem_get_ok:
    mov [total_mem_bytes], edx

    ; 准备进入保护模式

    ; 1. 打开A20地址线

    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

    ; 2. 加载gdt

    lgdt [gdt_ptr]

    ; 3. 将cr0的pe位置1

    mov eax, cr0 
    or eax, 0x00000001
    mov cr0, eax 

    jmp dword SELECTOR_CODE:p_mode_start    ; 刷新段描述缓冲寄存器与流水线

.error_hlt:
    hlt

[bits 32]
p_mode_start:

    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP

    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov eax, KERNEL_START_SECTOR    ; kernel.bin所在的扇区号
    mov ebx, KERNEL_BIN_BASE_ADDR   ; 磁盘读出后写入内存的起始地址
    mov ecx, 200                    ; 读取的扇区数

    call rd_disk_m_32
    
    ; 创建页目录及页表并初始化页内存位图
    call setup_page
    ; 将GDT就写入到内存gdt_ptr, 稍后用新地址重新加载 (低端1MB)
    sgdt [gdt_ptr]
    ; 将GDT描述符中视频(VIDEO)段描述符的段基址偏移3G (0xC0000000)
    mov ebx, [gdt_ptr + 2] ; GDT_BASE
    ; 参考上方定义的表 0x18是偏移到视频段描述符 +4是定位31~24位段基址
    or dword [ebx + 0x18 + 4], 0xc0000000

    ; 将gdt的基址偏移3G(0xC0000000) 使其成为内核所在的高地址
    add dword [gdt_ptr + 2], 0xC0000000
    add esp, 0xC0000000     ; 同样把栈指针映射到内核地址

    ; 把页目录地址赋值给cr3控制寄存器
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开cr0的PG位(第31位) 开启分页机制
    mov eax, cr0 
    or eax, 0x80000000
    mov cr0, eax 

    ; 开启分页后 用gdt的新地址重新加载
    lgdt [gdt_ptr]

    mov byte [gs:160], 'V'

    jmp SELECTOR_CODE:enter_kernel ; 32->32 原则不需要刷流水线，但确保什么 刷上
enter_kernel:
    call kernel_init
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT

;--------------------------------加载内核-------------------------------------
kernel_init:
    xor eax, eax 
    xor ebx, ebx    ; 记录程序头表位置
    xor ecx, ecx    ; 记录程序头表中的program header数量
    xor edx, edx    ; dx 记录program header的尺寸(e_phenesize)

    mov dx, [KERNEL_BIN_BASE_ADDR + 42] ; [42]处属性是e_phentsize 表示program header大小
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28] ; [28]处是e_phoff 表示第一个program header在文件中的偏移量

    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR + 44] ; [44]处是e_phnum 表示program header数量(有几个program header)

.each_segment:
    cmp byte [ebx + 0], PT_NULL ; 若p_type为PT_NULL, 说明此program header未使用
    je .PTNULL

    ; 为函数 memcpy 压入参数 从右往左入栈 memcpy(dst, src, size)
    push dword [ebx + 16] ; program_header[16]是p_filesz
    mov eax, [ebx + 4] ; ph[4]是p_offset
    add eax, KERNEL_BIN_BASE_ADDR ; 加上kernal二进制的加载地址
    push eax ; 压入 src
    push dword [ebx + 8] ; 压入 dst
    call memcpy
    add esp, 12
.PTNULL:
    add ebx, edx        ; edx为program header大小(e_phentsize) 这里指向下一个程序头
    loop .each_segment
    ret

; memcpy(dst, src, size) -> void
; 右往左压栈输入三个参数
memcpy:
    cld 
    push ebp
    mov ebp, esp 
    push ecx    ; 要用到ecx 备份外层的ecx

    mov edi, [ebp + 8]      ; dst
    mov esi, [ebp + 12]     ; src
    mov ecx, [ebp + 16]     ; size
    rep movsb               ; 逐字节拷贝

    pop ecx 
    pop ebp 
    ret

setup_page:
    ; 清空页目录表
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

;----------------------------------------------------------------------------
;                           内存分页设置 (Paging Setup)
;----------------------------------------------------------------------------
; 目标: 建立一个简单的页映射机制, 为进入保护模式并加载内核做准备。
;
; 内存布局:
;
;   +-----------------------+ <- PAGE_DIR_TABLE_POS (1MB)
;   |   页目录表 (PDT)      |
;   |   (4 KB)              |
;   +-----------------------+ <- PAGE_DIR_TABLE_POS + 4KB
;   |   第一个页表 (PT0)    |
;   |   (4 KB)              |
;   +-----------------------+
;
; 映射关系:
;   - 物理地址 0x00000000 - 0x003FFFFF (前 4MB)
;   - 映射到 -> 虚拟地址 0x00000000 - 0x003FFFFF
;   - 同时映射到 -> 虚拟地址 0xC0000000 - 0xC03FFFFF (内核空间)
;
; 实现步骤:
;   1. 清空页目录表 (PDT)。
;   2. 创建页目录项 (PDE), 将 PDT[0] 和 PDT[768] 指向同一个页表 (PT0)。
;   3. 在 PT0 中创建页表项 (PTE), 将前 4MB 物理地址进行一一映射。
;   4. 开启分页机制。
;----------------------------------------------------------------------------

; 创建页目录项(PDE)
.create_pde:    
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000     ; 第一个页表的位置及属性
    mov ebx, eax        ; ebx作基址为后续.create_pte做准备

    ; 第一个页表用于加载低端1MB, 将来也会加载内核 (预计不会超出1MB)
    ; 将页目录项 0 和 0xc00 存为第一个页表的地址
    ; 将 0 也一致映射到低端1MB是因为要兼容开启分页机制前与后的loader程序
    ; 0xc03fffff以下的地址和0x333fffff以下的地址指向相同的页表
    ; 为地址映射为内核地址做准备
    or eax, PG_US_U | PG_RW_W | PG_P ; U(所有特权级可访问) P(存在) W(可读可写)
    mov [PAGE_DIR_TABLE_POS + 0x000], eax ; 0x000 对应第一个页表
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax ; 0xc00 表示第768个页表, 0xc00以上的目录用于内核空间
    ; 0xc0000000 ~ 0xffffffff 1G 属于内核
    ; 0x0 ~ 0xbfffffff 3G 属于用户进程
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4092], eax ; 4092对应最后一个页目录项 最后一个目录项指向页目录表自己
                                         ; 为了将来能够动态操作表做铺垫
; 创建页表项(PTE)
    ; 加载低端1MB到第一个页表
    mov ecx, 256        ; 1M 低端内存 / 每页大小4k = 256
    mov esi, 0          
    mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
    mov [ebx + esi * 4], edx  ; 写入物理地址到页表项
    add edx, 4096   ; 计算下一个页表项对应的物理地址
    inc esi 
    loop .create_pte

; 创建内核其他页表的PDE(页目录项) (尽管一张就够, 但要真正实现内核被所有进程共享)
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000     ; 从第二个页表起，每个页表依序加载到页目录项
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254        ; 769 ~ 1022 的所有目录项的个数
    mov esi, 769        ; 起点
.create_kernel_pde:
    mov [ebx + esi * 4], eax
    inc esi 
    add eax, 0x1000
    loop .create_kernel_pde 
    ret

rd_disk_m_32: ; (eax, ebx, ecx) LBA扇区号 写入的内存地址 读入的扇区数
    mov esi, eax 
    mov di, cx  ; 备份eax cx

    ; 设置读取的扇区数
    mov dx, 0x1f2   ; 设置端口
    mov al, cl 
    out dx, al 

    mov eax, esi   ; 恢复 eax

    ; 将LBA地址存入 0x1f3 ~ 0x1f6
    ; 7~0
    mov dx, 0x1f3
    out dx, al 
    ; 15~8
    mov cl, 8
    shr eax, cl 
    mov dx, 0x1f4
    out dx, al 
    ; 23~16
    shr eax, cl 
    mov dx, 0x1f5 
    out dx, al 
    ; 27~24
    shr eax, cl 
    and al, 0x0f 
    or al, 0xe0 
    mov dx, 0x1f6 
    out dx, al

    ; 向0x1f7端口写入读命令, 0x20
    mov dx, 0x1f7 
    mov al, 0x20 
    out dx, al 

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
    mov [ebx], ax         ; 将数据写入bx (函数参数，要写入的目标内存地址)
    add ebx, 2            ; 移动bx到下一个字
    loop .go_on_read     ; 循环读取 (loop每次执行默认自减 cx)

    ret
    
