

#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>

// 特权级定义
auto constexpr RPL0 = 0;
auto constexpr RPL1 = 1;
auto constexpr RPL2 = 2;
auto constexpr RPL3 = 3;

// 描述符表指示器
auto constexpr TI_GDT = 0;
auto constexpr TI_LDT = 1;

// 段选择子定义
auto constexpr SELECTOR_K_CODE = (1 << 3) + (TI_GDT << 2) + RPL0;
auto constexpr SELECTOR_K_DATA = (2 << 3) + (TI_GDT << 2) + RPL0;
auto constexpr SELECTOR_K_STACK = SELECTOR_K_DATA;
auto constexpr SELECTOR_K_GS = (3 << 3) + (TI_GDT << 2) + RPL0;

// IDT 描述符属性
auto constexpr IDT_DESC_P = 1;
auto constexpr IDT_DESC_DPL0 = 0;
auto constexpr IDT_DESC_DPL3 = 3;
auto constexpr IDT_DESC_32_TYPE = 0xE;  // 32位的门
auto constexpr IDT_DESC_16_TYPE = 0x6;  // 16位的门,不会用到
                                        // 定义它只为了32位门区分
auto constexpr IDT_DESC_ATTR_DPL0 = 
    (IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE;
auto constexpr IDT_DESC_ATTR_DPL3 = 
    (IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE;

#endif //GLOBAL_H
