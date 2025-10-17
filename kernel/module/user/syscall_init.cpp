module;

#include <stdio.h>
#include <string.h>

export module syscall.init;

import syscall.utility;
import getpid;
import malloc;
import free;
import filesystem.syscall;
import fork;

export auto syscall_init();

auto constexpr syscall_nr = 32;

using sysfunc = void*;

extern "C" sysfunc syscall_table[syscall_nr]{};


auto syscall_init()
{
    puts("syscall_init start\n");
    syscall_table[+sysid::getpid] = (sysfunc)getpid;
    syscall_table[+sysid::write] = (sysfunc)write;
    syscall_table[+sysid::malloc] = (sysfunc)malloc;
    syscall_table[+sysid::free] = (sysfunc)free;
    syscall_table[+sysid::fork] = (sysfunc)fork;
    syscall_table[+sysid::read] = (sysfunc)read;
    syscall_table[+sysid::clear] = (sysfunc)clear;
    syscall_table[+sysid::putchar] = (sysfunc)putchar;
    puts("syscall_init done\n");
}