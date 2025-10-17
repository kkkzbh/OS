module;

#include <stdio.h>

export module thread.init;

import thread;
import schedule;
import process;
import sys;
import print;

export auto thread_init() -> void;

// init进程
auto init() -> void
{
    auto ret_pid = std::fork();
    if(ret_pid) {
        std::print("I am father, my pid is {}, child pid is {}\n",std::getpid(),ret_pid);
    } else {
        std::print("I am child, my pid is {}, ret pid is {}\n",std::getpid(),ret_pid);
    }
    std::print("hehehehehe\n  212112333 \n");
    while(true) {

    }
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