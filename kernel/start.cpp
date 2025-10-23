

#include <stdio.h>

auto init_all() -> void;
auto main() -> void;

import write_execution;

extern "C" auto start() -> void
{
    init_all();
    clear();
    write_execution();  // 切记仅运行一次！
    main();
    while(true) {

    }
}