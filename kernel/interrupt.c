

#include <interrupt.h>
#include <stdint.h>
#include <global.h>
#include <stdio.h>
#include <io.h>
#include <os.h>

auto constexpr IDT_DESC_CNT = 0x81;

auto constexpr EFLAGS_IF = 0x00000200; // eflags寄存器中的if位为 1

u32 syscall_handler();

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
extern intr_handler intr_entry_table[IDT_DESC_CNT]; // kernel.asm中的中断处理函数入口数组

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
    auto constexpr sysi = IDT_DESC_CNT - 1;
    make_idt_desc(&idt[sysi],IDT_DESC_ATTR_DPL3,syscall_handler);
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


    // 开启时钟与键盘中断
    outb(PIC_M_DATA, 0xfc);
    outb(PIC_S_DATA, 0xff);

    puts("pic_init done\n");
}

// 通用的中断处理函数，一般用于异常出现时的处理
void static general_intr_handler(u8 vec_nr)
{
    if(vec_nr == 0x27 || vec_nr == 0x2f) {
        // IRQ7与IRQ15 伪中断，无需处理 0x2f是从片8259A上的最后一个IRQ引脚，是保留项
        return;
    }

    // 清屏，从屏幕左上角打印异常信息，更清晰

    set_cursor(0);
    auto cursor_pos = 0;
    while(cursor_pos < 320) { // 25x80 清空前4行信息
        putchar(' ');
        ++cursor_pos;
    }
    set_cursor(0);
    puts("!!!!!!!    exception message begin      !!!!!!!\n");
    set_cursor(88); // 25 x 80的结构
    puts(intr_name[vec_nr]);
    if(vec_nr == 14) { // 缺页丢失
        u32 page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r"(page_fault_vaddr)); // cr2是存放造成page_fault的地址

        puts("\npage fault addr is ");
        puthex(page_fault_vaddr);
    }

    puts("\n!!!!!!!    exception message begin      !!!!!!!\n");

    while(true) {

    }
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

// 开中断 并返回先前的状态
intr_status intr_enable()
{
    if(INTR_ON == intr_get_status()) {
        return INTR_ON;
    }
    asm volatile("sti"); // 开中断 将IF位置位
    return INTR_OFF;
}

// 关中断 并返回先前的状态
intr_status intr_disable()
{
    if(INTR_OFF == intr_get_status()) {
        return INTR_OFF;
    }
    asm volatile("cli" : : : "memory");
            // 关中断 将IF位复位 设立memory内存屏障 防止cli后续的读写中断重排到cli之前
    return INTR_ON;
}

intr_status intr_set_status(intr_status status)
{
    if(status) {
        return intr_enable();
    }
    return intr_disable();
}

intr_status intr_get_status()
{
    u32 eflags;
    asm volatile("pushfl; popl %0" : "=g"(eflags));
    return EFLAGS_IF & eflags;
}

// 中断注册函数
void register_handler(u8 vector_no,intr_handler func)
{
    idt_table[vector_no] = func;
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