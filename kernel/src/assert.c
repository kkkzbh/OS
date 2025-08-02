

#include <assert.h>
#include <interrupt.h>
#include <stdio.h>

void panic_spin(char const* filename,int line,char const* func,char const* condition)
{
    intr_disable(); // 关中断 因为panic_span不一定只被assert调用 故在这里关中断

    puts("\n\n\n!!!!! error !!!!!\n");
    puts("filename: "); puts(filename); putchar('\n');
    puts("line: 0x"); puthex(line); putchar('\n');
    puts("function: "); puts(func); putchar('\n');
    puts("condition: "); puts(condition); putchar('\n');
    while(true) {

    }
}
