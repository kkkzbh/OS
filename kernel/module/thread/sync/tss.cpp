module;

#include <global.h>
#include <stdio.h>

export module tss;

import utility;
import task;

export auto tss_init() -> void;

struct tss
{

    // 更新 tss 中 esp0 字段的值为pthread的0级栈
    auto update(task const* pthread) -> void
    {
        esp0 = (u32*)((u32)pthread + PG_SIZE);
    }

    /* 任务状态段 (Task State Segment) */
    u32  backlink;            // 前一个任务的TSS 选择子
    u32* esp0;                // 特权级0 的栈指针
    u32  ss0;                 // 特权级0 的栈段选择子
    u32* esp1;                // 特权级1 的栈指针
    u32  ss1;                 // 特权级1 的栈段选择子
    u32* esp2;                // 特权级2 的栈指针
    u32  ss2;                 // 特权级2 的栈段选择子
    u32  cr3;                 // 页目录基址寄存器
    u32 (*eip)();             // 指令指针
    u32  eflags;              // 标志寄存器
    u32  eax;                 // 通用寄存器
    u32  ecx;
    u32  edx;
    u32  ebx;
    u32  esp;
    u32  ebp;
    u32  esi;
    u32  edi;
    u32  es;                  // 段寄存器
    u32  cs;
    u32  ss;
    u32  ds;
    u32  fs;
    u32  gs;
    u32  ldt;                 // LDT 选择子
    u32  trace;               // 调试用
    u32  io_base;             // I/O位图基址
};

export tss tss{};

auto make_gdt_desc(u32 const* desc_addr,u32 limit,u8 attr_low,u8 attr_high) -> gdt_desc
{
    auto desc_base = (u32)desc_addr;
    return {
        .limit_low_word         = u16(limit & 0x0000ffff),
        .base_low_word          = u16(desc_base & 0x0000ffff),
        .base_mid_byte          = u8((desc_base & 0x00ff0000) >> 16),
        .attr_low_byte          = attr_low,
        .limit_high_attr_high   = u8(((limit & 0x000f0000) >> 16) + attr_high),
        .base_high_byte         = u8(desc_base >> 24)
    };
}

// gdt中创建 tss 并重新加载gdt
auto tss_init() -> void
{
    puts("tss_init start\n");
    tss.ss0 = SELECTOR_K_STACK;
    tss.io_base = sizeof(tss);

    // gdt段基址 0x900 把tss放到第四个位置

    // gdt中 添加dpl为0的 TSS描述符
    *(gdt_desc*)0xc0000920 = make_gdt_desc((u32*)&tss,sizeof(tss) - 1,TSS_ATTR_LOW,TSS_ATTR_HIGH);

    // gdt中 添加dpl 为3的 代码段，数据段描述符
    *(gdt_desc*)0xc0000928 = make_gdt_desc(0,0xfffff,GDT_CODE_ATTR_LOW_DPL3,GDT_ATTR_HIGH);  // 用户代码段(第5个)
    *(gdt_desc*)0xc0000930 = make_gdt_desc(0,0xfffff,GDT_DATA_ATTR_LOW_DPL3,GDT_ATTR_HIGH);  // 用户数据段(第6个)

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