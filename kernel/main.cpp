

#include <stdio.h>

import memory;


extern "C" auto start() -> void
{

    puts("C++ extension dominate\n");

    auto addr = get_kernel_pages(3);

    puts("get_kernel_page start vaddr is: ");
    puthex(reinterpret_cast<u32>(addr));
    putchar('\n');

    while(true) {

    }
}