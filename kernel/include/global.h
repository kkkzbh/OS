#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

// ================ 特权级定义 ================
auto constexpr RPL0                   = 0;
auto constexpr RPL1                   = 1;
auto constexpr RPL2                   = 2;
auto constexpr RPL3                   = 3;

// ================ 描述符表指示器 ================
auto constexpr TI_GDT                 = 0;
auto constexpr TI_LDT                 = 1;

// ================ 段选择子定义 ================
auto constexpr SELECTOR_K_CODE        = (1 << 3) + (TI_GDT << 2) + RPL0;
auto constexpr SELECTOR_K_DATA        = (2 << 3) + (TI_GDT << 2) + RPL0;
auto constexpr SELECTOR_K_STACK       = SELECTOR_K_DATA;
auto constexpr SELECTOR_K_GS          = (3 << 3) + (TI_GDT << 2) + RPL0;
auto constexpr SELECTOR_TSS           = (4 << 3) + (TI_GDT << 2) + RPL0;

// 用户态段选择子 (第3个段描述符是显存, 第4个是TSS)
auto constexpr SELECTOR_U_CODE        = (5 << 3) + (TI_GDT << 2) + RPL3;
auto constexpr SELECTOR_U_DATA        = (6 << 3) + (TI_GDT << 2) + RPL3;
auto constexpr SELECTOR_U_STACK       = SELECTOR_U_DATA;

// ================ IDT 描述符属性 ================
auto constexpr IDT_DESC_P             = 1;
auto constexpr IDT_DESC_DPL0          = 0;
auto constexpr IDT_DESC_DPL3          = 3;
auto constexpr IDT_DESC_32_TYPE       = 0xE;    // 32位的门
auto constexpr IDT_DESC_16_TYPE       = 0x6;    // 16位的门,不会用到
                                                 // 定义它只为了32位门区分
auto constexpr IDT_DESC_ATTR_DPL0     = 
    (IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE;
auto constexpr IDT_DESC_ATTR_DPL3     = 
    (IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE;

// ================ GDT 描述符属性 ================
// 基本属性位
auto constexpr DESC_G_4K              = 1;
auto constexpr DESC_D_32              = 1;
auto constexpr DESC_L                 = 0;
auto constexpr DESC_AVL               = 0;
auto constexpr DESC_P                 = 1;
auto constexpr DESC_DPL0              = 0;
auto constexpr DESC_DPL1              = 1;
auto constexpr DESC_DPL2              = 2;
auto constexpr DESC_DPL3              = 3;

// S 字段
auto constexpr DESC_S_CODE            = 1;
auto constexpr DESC_S_DATA            = DESC_S_CODE;
auto constexpr DESC_S_SYS             = 0;

// 段类型 TYPE 字段
auto constexpr DESC_TYPE_CODE         = 0x8;    // 可执行/可读 代码段
auto constexpr DESC_TYPE_DATA         = 0x2;    // 可读/可写 数据段
auto constexpr DESC_TYPE_TSS          = 0x9;    // 可用 32 位 TSS 段

// 组合属性
auto constexpr GDT_ATTR_HIGH          =
    (DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4);

auto constexpr GDT_CODE_ATTR_LOW_DPL3 =
    (DESC_P << 7) + (DESC_DPL3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE;

auto constexpr GDT_DATA_ATTR_LOW_DPL3 =
    (DESC_P << 7) + (DESC_DPL3 << 5) + (DESC_S_DATA << 4) + DESC_TYPE_DATA;

// ================ TSS 描述符属性 ================
auto constexpr TSS_DESC_D             = 0;

auto constexpr TSS_ATTR_HIGH          =
    (DESC_G_4K << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4);

auto constexpr TSS_ATTR_LOW           =
    (DESC_P << 7) + (DESC_DPL0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS;

// ================ 数据结构定义 ================
// GDT 描述符结构
typedef struct
{
    u16 limit_low_word;
    u16 base_low_word;
    u8  base_mid_byte;
    u8  attr_low_byte;
    u8  limit_high_attr_high;
    u8  base_high_byte;
} gdt_desc;

// ================ EFLAGS 标志位定义 ================
auto constexpr EFLAGS_MBS             = (1 << 1);   // 此项必须要设置
auto constexpr EFLAGS_IF_1            = (1 << 9);   // if 为 1, 开中断
auto constexpr EFLAGS_IF_0            = 0;          // if 为 0, 关中断
auto constexpr EFLAGS_IOPL_3          = (3 << 12);
// IOPL3, 用于测试用户程序在非系统调用下进行 IO
auto constexpr EFLAGS_IOPL_0          = (0 << 12);  // IOPL0


__END_DECLS

#endif // _GLOBAL_H
