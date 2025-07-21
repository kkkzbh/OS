; loader 和 kernel
LOADER_BASE_ADDR       equ         0x900                         ; loader 加载地址 (考虑预留空白预防扩展)
LOADER_START_SECTOR    equ         0x2                           ; loader 起始扇区 (LBA) 
PAGE_DIR_TABLE_POS     equ         0x100000          

; gdt 描述符属性
DESC_G_4K              equ         1_00000000000000000000000b    ; G位, 1=段界限单位为4KB
DESC_D_32              equ         1_0000000000000000000000b     ; D/B位, 1=32位段
DESC_L                 equ         0_000000000000000000000b      ; 64位代码标记 标记为0表示32位
DESC_AVL               equ         0_00000000000000000000b       ; AVL位, 系统软件可用位

DESC_LIMIT_CODE2       equ         1111_0000000000000000b        ; 代码段段界限的第二部分(19~16位)
DESC_LIMIT_DATA2       equ         DESC_LIMIT_CODE2              ; 数据段段界限的第二部分(19~16位)
DESC_LIMIT_VIDEO2      equ         0000_000000000000000b         ; 显存段段界限的第二部分(19~16位)
DESC_P                 equ         1_000000000000000b            ; P位, 1=段存在
DESC_DPL_0             equ         00_0000000000000b             ; DPL位, 特权级0(最高特权级)
DESC_DPL_1             equ         01_0000000000000b             ; DPL位, 特权级1
DESC_DPL_2             equ         10_0000000000000b             ; DPL位, 特权级2
DESC_DPL_3             equ         11_0000000000000b             ; DPL位, 特权级3(最低特权级)
DESC_S_CODE            equ         1_000000000000b               ; S位, 1=非系统段(代码和数据段)
DESC_S_DATA            equ         DESC_S_CODE                   ; S位, 1=非系统段(代码和数据段)
DESC_S_sys             equ         0_000000000000b               ; S位, 0=系统段
DESC_TYPE_CODE         equ         1000_00000000b                ; 代码段 可执行, 非一致, 不可读, 已访问位清0
DESC_TYPE_DATA         equ         0010_00000000b                ; 数据段 不可执行, 向上扩展, 可写, 已访问位清0

; 段描述符高4字节的定义
; GDT中的段描述符由8个字节组成, HIGH4表示高4字节部分
; 包含了段基址的高24位(31~8位)和各种段属性标志

DESC_CODE_HIGH4        equ         (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                                   DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
                                   DESC_P + DESC_DPL_0 + DESC_S_CODE + \
                                   DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4        equ         (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                                   DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
                                   DESC_P + DESC_DPL_0 + DESC_S_DATA + \
                                   DESC_TYPE_DATA + 0x00

DESC_VIDEO_HIGH4       equ         (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
                                   DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + \
                                   DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0B

;-------------- 选择子属性 ---------------
RPL0                   equ         00b                           ; 请求特权级0
RPL1                   equ         01b                           ; 请求特权级1
RPL2                   equ         10b                           ; 请求特权级2
RPL3                   equ         11b                           ; 请求特权级3
TI_GDT                 equ         000b                          ; GDT指示位
TI_LDT                 equ         100b                          ; LDT指示位

;-------------- 页表中的属性 ---------------

PG_P                   equ         1b                            ; 存在位
PG_RW_R                equ         00b                           ; 读写位
PG_RW_W                equ         01b                           ; 读写位
PG_US_S                equ         000b                          ; 系统级
PG_US_U                equ         100b                          ; 用户级

