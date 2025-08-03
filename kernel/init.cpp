

#include <init.h>
#include <stdio.h>
#include <interrupt.h>
#include <time.h>
#include <assert.h>

import memory;
import thread;

extern "C" void init_all()
{
    puts("init_all\n");
    idt_init();         // 初始化 中断
    mem_init();         // 初始化 内存管理系统
    thread_init();      // 初始化 线程环境
    timer_init();       // 初始化 PIT
}
