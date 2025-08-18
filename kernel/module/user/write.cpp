module;

#include <string.h>

export module write;

import console;

export auto write(char const* str) -> u32
{
    console::write(str);
    return strlen(str);
}