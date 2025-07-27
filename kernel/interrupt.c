

#include <interrupt.h>
#include <stdint.h>
#include <global.h>
#include <stdio.h>
#include <io.h>

auto constexpr IDT_DESC_CNT = 0x21;

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
        make_idt_desc(&idt[i],IDT_DESC_ATTR_DPL0,intr_entry_table[i]);
    }
    puts("idt_desc_init done\n");
}

auto constexpr PIC_M_CTRL = 0x20; // 主片控制端口
auto constexpr PIC_M_DATA = 0x21; // 从片控制端口
auto constexpr PIC_S_CTRL = 0xa0; // 从片控制端口
auto constexpr PIC_S_DATA = 0xa1; // 从片数据端口

void static pic_init()
{
    // 初始化主片
    outb(PIC_M_CTRL,0x11); // ICW1: 边沿触发，级联8259，需要ICW4
    outb(PIC_M_DATA,0x20); // ICW2: 起始中断向量号为 0x20
    outb(PIC_M_DATA,0x04); // ICW3: IR2 接从片
    outb(PIC_M_DATA,0x01); // ICW4: 8086模式，正常EOI

    // 初始化从片
    outb(PIC_S_CTRL,0x11); // ICW1: 边沿触发，级联8259，需要ICW4
    outb(PIC_S_DATA,0x28); // ICW2: 起始中断向量号为 0x28
    outb(PIC_S_DATA,0x02); // ICW3: 设置从片连接到主片的 IR2 引脚
    outb(PIC_S_DATA,0x01); // ICW4: 8086模式，正常EOI

    // 打开主片上的 IR0，即时钟中断
    outb(PIC_M_DATA,0xfe);
    outb(PIC_S_DATA,0xff);

    puts("pic_init done\n");
}

void idt_init()
{
    puts("idt_init start\n");
    idt_desc_init();
    pic_init();
    // 加载idt
    auto idt_operand = (sizeof(idt) - 1) | ((u64)(u32)idt << 16); // 低16位尺寸，高32位线性基地址 加载idtg
    asm volatile("lidt %0": : "m"(idt_operand));
    puts("idt_init done\n");
}