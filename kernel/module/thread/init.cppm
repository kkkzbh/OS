module;

#include <stdio.h>
#include <assert.h>

export module thread.init;

import thread;
import schedule;
import process;
import sys;
import print;
import shell;
import string;

export auto thread_init() -> void;

// init进程
auto init() -> void
{
    auto ret_pid = std::fork();
    if(ret_pid) {   // 父进程
        while(true) {
            int status;
            // 不断回收僵尸进程
            auto child_pid = std::wait(status);  // 使用 std::wait 系统调用
            std::println("init: recieve a child, pid {}, status {}",child_pid,status);
        }
    }
    // 子进程
    shell();
    PANIC("init: should not be here");
}

auto thread_init() -> void
{
    puts("thread_init start");

    // 创建第一个用户进程:init
    process_execute((void*)init,"init");

    // 将当前main函数创建为线程
    make_main_thread();

    // 创建idle线程
    idle_thread = thread_start("idle",10,idle,nullptr);

    puts("thread_init done\n");
}