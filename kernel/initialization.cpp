module;

#include <initialization.h>
#include <stdio.h>
#include <interrupt.h>
#include <time.h>
#include <assert.h>
#include <keyboard.h>

auto call_global_constructors() -> void
{
    typedef void (*constrctror)();
    extern constrctror __init_array_start[];
    extern constrctror __init_array_end[];
    for(auto p = __init_array_start; p != __init_array_end; ++p) {
        (*p)();
    }

}

export module init;

import memory;
import tss;
import thread;


export extern "C" void init_all()
{
    puts("init_all\n");
    call_global_constructors(); // 调用C++全局构造函数

    idt_init();         // 初始化 中断
    mem_init();         // 初始化 内存管理系统
    thread_init();      // 初始化 线程环境
    timer_init();       // 初始化 PIT
    keyboard_init();    // 初始化 键盘中断
    tss_init();         // 初始化 tss
}