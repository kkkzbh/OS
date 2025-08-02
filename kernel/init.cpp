

#include <init.h>
#include <stdio.h>
#include <interrupt.h>
#include <time.h>
#include <assert.h>

import memory;

extern "C" void init_all()
{
    puts("init_all\n");
    idt_init();         // 初始化 中断
    timer_init();       // 初始化 PIT
    mem_init();         // 初始化 内存管理系统
}
