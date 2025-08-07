

#include <stdio.h>
#include <interrupt.h>

import console;
import thread;
import process;

int v1;
int v2;

void init_all();

extern "C" auto main() -> void
{
    init_all();


    puts("C++ extension dominate\n");

    // thread_start("kthread_a",31,
    //              [](void* arg) -> void {
    //                  auto s = static_cast<char*>(arg);
    //                  while(true) {
    //                      console::write(" v1 = ");
    //                      console::puth(v1);
    //                  }
    //              }
    //              ,const_cast<char*>("ArgX   ")
    //          );
    //
    // thread_start("kthread_b",8,
    // [](void* arg) -> void {
    //     auto s = static_cast<char*>(arg);
    //     while(true) {
    //         console::write(" v2 = ");
    //         console::puth(v2);
    //     }
    // }
    // ,const_cast<char*>("ArgB   "));

    process_execute((void*)+[] {
        while(true) {
            ++v1;
        }
    },"user_prog_a");

    // process_execute((void*)+[] {
    //     while(true) {
    //         ++v2;
    //     }
    // },"user_prog_b");

    intr_enable();


    while(true) {
        console::puth(v1);
    }
}