module;

#include <stdio.h>

export module thread:init;

import :exec;

export auto thread_init() -> void;


auto thread_init() -> void
{
    puts("thread_init start");
    make_main_thread();
    puts("thread_init done\n");
}