

#include <stdio.h>

import memory;
import thread;

extern "C" auto start() -> void
{

    puts("C++ extension dominate\n");

    auto addr = get_kernel_pages(3);

    puts("get_kernel_page start vaddr is: ");
    puthex(reinterpret_cast<u32>(addr));
    putchar('\n');

    auto v = reinterpret_cast<int*>(addr);
    *v = 123;
    puts("modify the kernel memory : v = ");
    puthex(*v);
    putchar('\n');

    thread_start("kthread_a",31,
        [](void* arg) -> void {
            auto s = static_cast<char*>(arg);
            while(1) {
                puts(s);
            }
        }
        ,const_cast<char*>("Arg666   "));

    while(true) {

    }
}