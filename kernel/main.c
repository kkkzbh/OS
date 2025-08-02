

#include <stdio.h>
#include <init.h>
#include <assert.h>

void start();

int kkkzbh()
{
    // 在init_all()之前调用C++全局构造函数
    // call_global_constructors();

    puts("kkkzbh says: Hello OS\n");
    puthex(0x123);
    putchar('\n');


    init_all();

    start();

    ASSERT(1 == 2);

    return 0;
}
