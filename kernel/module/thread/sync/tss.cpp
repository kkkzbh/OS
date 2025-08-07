module;

#include <global.h>
#include <stdio.h>

export module sync:tss;

import utility;
import :task;

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
