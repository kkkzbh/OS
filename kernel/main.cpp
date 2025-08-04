

#include <stdio.h>
#include <interrupt.h>

import memory;
import thread;
import console;

struct test
{
    test()
    {
        puts("  ***********  test constrctor function ! ********\n");
    }
} t{}, t2{};

extern "C" auto start() -> void
{

    puts(" ***global_constructors! ok ***\n");
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
            while(true) {
                console::write(s);
            }
        }
        ,const_cast<char*>("ArgX   ")
    );

    thread_start("kthread_b",8,
    [](void* arg) -> void {
        auto s = static_cast<char*>(arg);
        while(true) {
            console::write(s);
        }
    }
    ,const_cast<char*>("ArgB   "));

    intr_enable();


    while(true) {
        console::write("Main   ");
    }
}