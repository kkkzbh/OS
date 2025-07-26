

#include <interrupt.h>
#include <stdint.h>
#include <global.h>
#include <stdio.h>

auto constexpr IDT_DESC_CNT = 0x3;

// 中断门描述符结构体
typedef struct gate_desc
{
    u16     func_offset_low_word;
    u16     selector;
    u8      dcount;
    u8      attribute;
    u16     func_offset_high_word;
} gate_desc;

gate_desc idt[IDT_DESC_CNT]; // 中断描述符表
extern intr_handler intr_entry_table[IDT_DESC_CNT];

void static make_idt_desc(gate_desc* gdesc,u8 attr,intr_handler function)
{
    *gdesc = (gate_desc) {
        .func_offset_low_word = (u32)function & 0x0000FFFF,
        .func_offset_high_word = ((u32) function & 0xFFFF0000) >> 16,
        .dcount = 0,
        .attribute = attr,
        .selector = SELECTOR_K_CODE
    };
}

void static idt_desc_init()
{
    for(auto i = 0; i != IDT_DESC_CNT; ++i) {
        make_idt_desc(&idt[i],IDT_DESC_DPL0,intr_entry_table[i]);
    }
    puts("idt_desc_init done\n");
}

void idt_init()
{
    puts("idt_init start\n");
    idt_desc_init();
    // pic_init();
    // 加载idt
    // auto idt_operand = (sizeof(idt) - 1) | (u64)((u32)idt << 16);
    // asm volatile("lidt %0": : "m"(idt_operand));
    puts("idt_init done\n");
}