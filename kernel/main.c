

#include <stdio.h>
#include <init.h>
#include <assert.h>

void start();


int kkkzbh()
{

    puts("kkkzbh says: Hello OS\n");
    puthex(0x123);
    putchar('\n');

    init_all();

    start();

    ASSERT(1 == 2);

    return 0;
}
