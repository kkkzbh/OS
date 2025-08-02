

#include <stdint.h>
#include <time.h>
#include <io.h>
#include <stdio.h>

auto constexpr IRQ0_FREQUENCY   =          100;
auto constexpr INPUT_FREQUENCY  =          1193180;
auto constexpr COUNTER0_VALUE   =          INPUT_FREQUENCY / IRQ0_FREQUENCY;
auto constexpr COUNTER0_PORT    =          0x40;
auto constexpr COUNTER0_NO      =          0;
auto constexpr COUNTER_MODE     =          2;
auto constexpr READ_WRITE_LATCH =          3;
auto constexpr PIT_CONTROL_PORT =          0x43;


void static frequency_set (
    u8 counter_port,
    u8 counter_no,      // 计数器编号
    u8 rwl,             // 读写锁属性
    u8 counter_mode,    // 工作模式
    u16 counter_value   // 初值
)
{
    // 往控制寄存器端口 0x43 写入控制字
    outb(PIT_CONTROL_PORT,counter_no << 6 | rwl << 4 | counter_mode << 1);
    // 先写入初值的低 8 位 再写入高 8 位
    outb(counter_port, counter_value);
    outb(counter_port, counter_value >> 8);
}

void timer_init()
{
    puts("timer_init start\n");
    frequency_set (
        COUNTER0_PORT,
        COUNTER0_NO,
        READ_WRITE_LATCH,
        COUNTER_MODE,
        COUNTER0_VALUE
    );
    puts("timer_init done\n");

}