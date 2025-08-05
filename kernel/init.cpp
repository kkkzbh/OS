

#include <init.h>
#include <stdio.h>
#include <interrupt.h>
#include <time.h>
#include <assert.h>
#include <keyboard.h>

import memory;
import thread;

void call_global_constructors()
{
    typedef void (*constrctror)();
    extern constrctror __init_array_start[];
    extern constrctror __init_array_end[];
    for(auto p = __init_array_start; p != __init_array_end; ++p) {
        (*p)();
    }

}

extern "C" void init_all()
{
    puts("init_all\n");
    call_global_constructors();
    idt_init();         // 初始化 中断
    mem_init();         // 初始化 内存管理系统
    thread_init();      // 初始化 线程环境
    timer_init();       // 初始化 PIT
    keyboard_init();    // 初始化 键盘中断
}
