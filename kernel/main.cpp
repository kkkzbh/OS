

#include <stdio.h>
#include <interrupt.h>
void init_all();

import console;
import thread;
import process;
import sys;
import syscall.init;
import printf;
import getpid;

int prog_a_pid;
int prog_b_pid;

extern "C" auto main() -> void
{
    init_all();

    process_execute((void*)+[] {
        prog_a_pid = sys::getpid();
        std::printf(" prog_a_pid:0x%x\n",prog_a_pid);
        while(true) {

        }
    },"user_prog_a");

    process_execute((void*)+[] {
        prog_b_pid = sys::getpid();
        std::printf(" prog_b_pid:0x%x\n",prog_b_pid);
        while(true) {

        }
    },"user_prog_b");

    intr_enable();

    console::write("  main_pid:0x");
    console::puth(getpid());
    console::putc('\n');

    thread_start("k_thread_a",31,[](void* arg) {
        auto para = (char*)arg;
        console::write(" thread_a_pid:0x");
        console::puth(getpid());
        console::putc('\n');
        while(true) {

        }
    },(void*)"argA ");

    thread_start("k_thread_b",31,[](void* arg) {
        auto para = (char*)arg;
        console::write(" thread_b_pid:0x");
        console::puth(getpid());
        console::putc('\n');
        while(true) {

        }
    },(void*)"argB ");

    while(true) {

    }
}