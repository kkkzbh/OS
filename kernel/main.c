

#include <stdio.h>
#include <init.h>

int kkkzbh()
{
    puts("kkkzbh says: Hello OS\n");
    puthex(0x123);
    putchar('\n');

    init_all();
    asm volatile("sti");

    while(true) {}
}
