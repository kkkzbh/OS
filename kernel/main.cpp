

#include <stdio.h>
#include <interrupt.h>

import memory;
import thread;

struct test
{
    test()
    {
        puts("  ***********  test constrctor function ! ********\n");
    }
};

test t{};
test t2{};

void call_global_constructors()
{
    typedef void (*constrctror)();
    extern constrctror __init_array_start[];
    extern constrctror __init_array_end[];
    for(auto p = __init_array_start; p != __init_array_end; ++p) {
        (*p)();
    }

}

extern "C" auto start() -> void
{

    call_global_constructors();
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

    // thread_start("kthread_a",31,
    //     [](void* arg) -> void {
    //         auto s = static_cast<char*>(arg);
    //         while(true) {
    //             intr_disable();
    //             puts(s);
    //             intr_enable();
    //         }
    //     }
    //     ,const_cast<char*>("ArgX   ")
    // );
    //
    // thread_start("kthread_b",8,
    // [](void* arg) -> void {
    //     auto s = static_cast<char*>(arg);
    //     while(true) {
    //         intr_disable();
    //         puts(s);
    //         intr_enable();
    //     }
    // }
    // ,const_cast<char*>("ArgB   "));
    //
    // intr_enable();


    while(true) {
        // intr_disable();
        // puts("Main    ");
        // intr_enable();
    }
}