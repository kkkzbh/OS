

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
char* intr_name[IDT_DESC_CNT]; // 保存异常名字
intr_handler idt_table[IDT_DESC_CNT]; // 中断处理程序数组，程序入口
extern intr_handler intr_entry_table[IDT_DESC_CNT]; // kernel.sam中的中断处理函数入口数组

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

// 通用的中断处理函数，一般用于异常出现时的处理
void static general_intr_handler(u8 vec_nr)
{
    if(vec_nr == 0x27 || vec_nr == 0x2f) {
        // IRQ7与IRQ15 伪中断，无需处理 0x2f是从片8259A上的最后一个IRQ引脚，是保留项
        return;
    }
    puts("int vector : 0x");
    puthex(vec_nr);
    putchar('\n');
}

// 完成一般中断处理函数注册及异常名称注册
void static exception_init()
{
    for(auto i = 0; i != IDT_DESC_CNT; ++i) {
        idt_table[i] = general_intr_handler; // idt_table 进入中断后，根据中断向量号调用
        intr_name[i] = "unknown"; // 默认注册为general处理函数，名字先默认统一为 unknown
        // 后续通过中断注册函数 修改 idt_table (加上name)
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项,未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

void idt_init()
{
    puts("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();
    // 加载idt
    auto idt_operand = (sizeof(idt) - 1) | ((u64)(u32)idt << 16); // 低16位尺寸，高32位线性基地址 加载idtg
    asm volatile("lidt %0": : "m"(idt_operand));
    puts("idt_init done\n");
}