module;

#include <global.h>
#include <stdio.h>

export module sync:init;

import :tss;

export auto tss_init() -> void;

// gdt中创建 tss 并重新加载gdt
auto tss_init() -> void
{
    puts("tss_init start\n");
    tss.ss0 = SELECTOR_K_STACK;
    tss.io_base = sizeof(tss);

    // gdt段基址 0x900 把tss放到第四个位置

    // gdt中 添加dpl为0的 TSS描述符
    *(gdt_desc*)0xc0000920 = make_gdt_desc((u32*)&tss,sizeof(tss) - 1,TSS_ATTR_LOW,TSS_ATTR_HIGH);

    // gdt中 添加dpl 为3的 数据段，代码段描述符
    *(gdt_desc*)0xc0000928 = make_gdt_desc(0,0xfffff,GDT_CODE_ATTR_LOW_DPL3,GDT_ATTR_HIGH);
    *(gdt_desc*)0xc0000928 = make_gdt_desc(0,0xfffff,GDT_DATA_ATTR_LOW_DPL3,GDT_ATTR_HIGH);

    // gdt 16位的 limit 32位的段基址

    struct __attribute__((packed))
    {
        u16 limit;
        u32 base_addr;
    } gdt_operand{ 8 * 7 - 1,0xc0000900 };

    asm volatile("lgdt %0" : : "m"(gdt_operand));
    asm volatile("ltr %w0" : : "r"(SELECTOR_TSS));

    puts("tss_init and ltr done\n");

}