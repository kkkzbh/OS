module;

#include <string.h>

export module print;

import format;
import utility;
import sys;
import file.structure;


namespace std
{

    char buf[1024]{};

    export template<typename... Args>
    auto print(char const* format,Args&&... args) -> u32
    {
        auto c = format_to(buf,format,forward<Args>(args)...);
        buf[c] = '\0';
        return std::write(stdout,buf,strlen(buf));
    }

    export template<typename... Args>
    auto sprintf(char* buf,char const* format,Args&&... args) -> u32
    {
        return format_to(buf,format,forward<Args>(args)...);
    }
}