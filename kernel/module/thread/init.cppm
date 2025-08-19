module;

#include <stdio.h>

export module thread:init;

import :exec;
import schedule;

export auto thread_init() -> void;


auto thread_init() -> void
{
    puts("thread_init start");
    make_main_thread();
    idle_thread = thread_start("idle",10,idle,nullptr);
    puts("thread_init done\n");
}