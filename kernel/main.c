

#include <stdio.h>
#include <init.h>
#include <assert.h>

// // C++全局构造函数支持
// extern void (*__init_array_start[])();
// extern void (*__init_array_end[])();
//
// // C++纯虚函数调用处理
// void __cxa_pure_virtual() {
//     PANIC("Pure virtual function called!\n");
//     while(1) {
//
//     }
// }
//
// // 调用全局构造函数
// void call_global_constructors() {
//     void (**p)();
//     for (p = __init_array_start; p != __init_array_end; ++p) {
//         (*p)();
//     }
// }

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
