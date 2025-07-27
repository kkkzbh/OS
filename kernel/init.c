

#include <init.h>
#include <stdio.h>
#include <interrupt.h>

void init_all()
{
    puts("init_all\n");
    idt_init();
}
