
#include <stdio.h>
#include <interrupt.h>
#include <time.h>
#include <assert.h>
#include <keyboard.h>

import alloc;
import tss;
import syscall.init;
import storage.init;
import storage.regression;
import filesystem.init;
import console;
import thread.init;
import iobuf;

auto call_global_constructors() -> void
{
    typedef void (*constrctror)();
    extern constrctror __init_array_start[];
    extern constrctror __init_array_end[];
    for(auto p = __init_array_start; p != __init_array_end; ++p) {
        (*p)();
    }

}

auto clear_bss() -> void
{
    extern char __bss_start[];
    extern char _end[];
    for(auto p = __bss_start; p != _end; ++p) {
        *p = 0;
    }
}

auto init_all() -> void
{
    puts("init_all\n");
    clear_bss();                // 清零 BSS 段，显式初始化的全局状态依赖这一点。
    call_global_constructors(); // 仅保留给编译器 / modules 运行时初始化，不再承担关键子系统状态构造。

    idt_init();         // 初始化 中断
    mem_init();         // 初始化 内存管理系统
    thread_init();      // 初始化 线程环境
    console_init();     // 初始化控制台运行时状态
    timer_init();       // 初始化 PIT
    iobuf_init();       // 初始化键盘输入缓冲
    keyboard_init();    // 初始化 键盘中断
    tss_init();         // 初始化 tss
    syscall_init();     // 初始化 系统调用

    // 初始化硬盘要开中断
    intr_enable();
    storage_init();     // 初始化 存储

    if(disk_regression_mode_requested()) {
        if(not disk_regression_mode_case()) {
            start_init_process();
        }
        return;
    }

    filesystem_init();  // 初始化 文件系统

}
