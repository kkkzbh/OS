

#include <../time.h>
#include <assert.h>

import utility;
import thread;

u32 ticks;  // 内核自中断开启以来总共的嘀嗒数

// 时钟的中断处理函数
extern "C" auto intr_time_handler() -> void
{
    auto cur_thread = running_thread();
    ASSERT(cur_thread->stack_magic == 0x19870916);  // 魔数 判断栈是否溢出

    ++cur_thread->elapsed_ticks; // 记录此线程占用的 cpu 时间
    ++ticks;

    if(cur_thread->ticks == 0) { // 进程时间片用完，调度新进程上cpu
        schedule();
    } else {
        --cur_thread->ticks;
    }

}