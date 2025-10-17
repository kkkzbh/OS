
#include <stdio.h>

auto init_all() -> void;
auto main() -> void;


extern "C" auto start() -> void
{
    init_all();
    clear();
    main();
    while(true) {

    }
}