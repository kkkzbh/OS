

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
import malloc;
import list;

int prog_a_pid;
int prog_b_pid;

extern "C" auto main() -> void
{
    init_all();

    // process_execute((void*)+[] {
    //     prog_a_pid = sys::getpid();
    //     char const* name = "prog_a";
    //     std::printf(" I am %s, my pid: %d%c",name,prog_a_pid,'\n');
    //     while(true) {
    //
    //     }
    // },"user_prog_a");
    //
    // process_execute((void*)+[] {
    //     prog_b_pid = sys::getpid();
    //     char const* name = "prog_b";
    //     std::printf(" I am %s, my pid: %d%c",name,prog_b_pid,'\n');
    //     while(true) {
    //
    //     }
    // },"user_prog_b");

    intr_enable();

    // console::write("  main_pid:0x");
    // console::puth(getpid());
    // console::putc('\n');

    thread_start("k_thread_a",31,[](void* arg) {
        auto para = (char*)arg;
        // console::write(" thread_a_pid:0x");
        // console::puth(getpid());
        // console::putc('\n');
        auto addr = malloc(33);
        console::write(" I am thread_a, std::malloc(33), addr is 0x");
        console::puth((int)addr);
        console::putc('\n');
        while(true) {

        }
    },(void*)"argA ");

    thread_start("k_thread_b",31,[](void* arg) {
        auto para = (char*)arg;
        // console::write(" thread_b_pid:0x");
        // console::puth(getpid());
        // console::putc('\n');
        auto addr = malloc(63);
        console::write(" I am thread_b, std::malloc(63), addr is 0x");
        console::puth((int)addr);
        console::putc('\n');
        while(true) {

        }
    },(void*)"argB ");

    while(true) {

    }
}