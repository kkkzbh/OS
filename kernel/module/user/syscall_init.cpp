module;

#include <stdio.h>

export module syscall.init;

import syscall.utility;
import schedule;

export auto syscall_init();

export extern "C" auto getpid() -> u32;

auto constexpr syscall_nr = 32;

using sysfunc = void*;

extern "C" sysfunc syscall_table[syscall_nr]{};

extern "C" auto getpid() -> u32
{
    return running_thread()->pid;
}

auto syscall_init()
{
    puts("syscall_init start\n");
    syscall_table[+sysid::getpid] = (sysfunc)getpid;
    puts("syscall_init done\n");
}