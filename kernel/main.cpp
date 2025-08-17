

#include <stdio.h>
#include <interrupt.h>

import console;
import thread;
import process;

int v1;
int v2;

void init_all();

// 测试用户进程，类似原书的u_prog_a，访问全局变量
extern "C" void test_user_prog() {
    while(true) {
        ++v1;
    }
}

extern "C" auto main() -> void
{
    init_all();

    puts("C++ extension dominate\n");

    thread_start("kthread_a",31,
                 [](void* arg) -> void {
                     auto s = static_cast<char*>(arg);
                     while(true) {
                         console::write(" v1 = ");
                         console::puth(v1);
                     }
                 }
                 ,const_cast<char*>("ArgX   ")
             );

    thread_start("kthread_b",8,
    [](void* arg) -> void {
        auto s = static_cast<char*>(arg);
        while(true) {
            console::write(" v2 = ");
            console::puth(v2);
        }
    }
    ,const_cast<char*>("ArgB   "));

    process_execute((void*)test_user_prog, "user_prog_a");

    process_execute((void*)+[] {
        while(true) {
            ++v2;
        }
    },"user_prog_b");

    intr_enable();

    while(true) {

    }
}