

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
import free;
import format;
import string;
import string.format;
import array;
import array.format;

// int prog_a_pid;
// int prog_b_pid;

extern "C" auto main() -> void
{


    init_all();

    // intr_enable();
    //
    // process_execute((void*)+[] {
    //     auto addr1 = std::malloc(256);
    //     auto addr2 = std::malloc(255);
    //     auto addr3 = std::malloc(254);
    //     std::printf(" prog_a malloc addr:0x%x, 0x%x, 0x%x\n",addr1,addr2,addr3);
    //     auto cpu_delay = 100000000;
    //     while(cpu_delay--) {
    //
    //     }
    //     std::free(addr1);
    //     std::free(addr2);
    //     std::free(addr3);
    //     while(true) {
    //
    //     }
    // },"user_prog_a");

    // process_execute((void*)+[] {
    //     auto addr1 = std::malloc(256);
    //     auto addr2 = std::malloc(255);
    //     auto addr3 = std::malloc(254);
    //     std::printf(" prog_b malloc addr:0x%x, 0x%x, 0x%x\n",addr1,addr2,addr3);
    //     auto cpu_delay = 100000000;
    //     while(cpu_delay--) {
    //
    //     }
    //     std::free(addr1);
    //     std::free(addr2);
    //     std::free(addr3);
    //     while(true) {
    //
    //     }
    // },"user_prog_b");
    //
    // thread_start("k_thread_a",31,[](void* arg) {
    //     auto para = (char*)arg;
    //     auto addr1 = malloc(256);
    //     auto addr2 = malloc(255);
    //     auto addr3 = malloc(254);
    //     console::write(" thread_a malloc addr:0x");
    //     console::puth((int)addr1);
    //     console::putc(',');
    //     console::puth((int)(addr2));
    //     console::putc(',');
    //     console::puth((int)addr3);
    //     console::putc('\n');
    //
    //     auto cpu_delay = 1000000000;
    //
    //     while(cpu_delay--) {
    //
    //     }
    //     free(addr1);
    //     free(addr2);
    //     free(addr3);
    //
    //     while(true) {
    //
    //     }
    // },(void*)"argA ");
    //
    // thread_start("k_thread_b",31,[](void* arg) {
    //     auto para = (char*)arg;
    //     auto addr1 = malloc(256);
    //     auto addr2 = malloc(255);
    //     auto addr3 = malloc(254);
    //     console::write(" thread_b malloc addr:0x");
    //     console::puth((int)addr1);
    //     console::putc(',');
    //     console::puth((int)(addr2));
    //     console::putc(',');
    //     console::puth((int)addr3);
    //     console::putc('\n');
    //
    //     auto cpu_delay = 1000000000;
    //
    //     while(cpu_delay--) {
    //
    //     }
    //     free(addr1);
    //     free(addr2);
    //     free(addr3);
    //     while(true) {
    //
    //     }
    // },(void*)"argB ");

    while(true) {

    }
}