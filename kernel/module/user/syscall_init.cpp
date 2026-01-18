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
import ps;
import exec;
import wait;
import exit;

export auto syscall_init();

auto constexpr syscall_nr = 32;

using sysfunc = void*;

extern "C" sysfunc syscall_table[syscall_nr]{};

auto syscall_init()
{
    puts("syscall_init start\n");
    #define install(X) syscall_table[+sysid::X] = (sysfunc)X
    install(getpid);
    install(write);
    install(malloc);
    install(free);
    install(fork);
    install(read);
    install(clear);
    install(putchar);
    install(getcwd);
    install(open);
    install(close);
    install(lseek);
    install(unlink);
    install(mkdir);
    install(opendir);
    install(closedir);
    install(chdir);
    install(rmdir);
    install(readdir);
    install(rewinddir);
    install(stat);
    install(ps);
    install(exec);
    install(wait);
    install(exit);
    puts("syscall_init done\n");
}