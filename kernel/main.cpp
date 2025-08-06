module;

#include <stdio.h>
#include <interrupt.h>

export module os;

import console;
import init;

extern "C" auto start() -> void
{
    init_all();

    puts("C++ extension dominate\n");

    // thread_start("kthread_a",31,
    //     [](void* arg) -> void {
    //         auto s = static_cast<char*>(arg);
    //         while(true) {
    //             console::write(s);
    //         }
    //     }
    //     ,const_cast<char*>("ArgX   ")
    // );
    //
    // thread_start("kthread_b",8,
    // [](void* arg) -> void {
    //     auto s = static_cast<char*>(arg);
    //     while(true) {
    //         console::write(s);
    //     }
    // }
    // ,const_cast<char*>("ArgB   "));

    intr_enable();


    while(true) {
        //console::write("Main   ");
    }
}