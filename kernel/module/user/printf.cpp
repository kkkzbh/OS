

export module printf;

import format;
import utility;
import sys;

namespace std
{
    export template<typename... Args>
    auto printf(char const* format,Args&&... args) -> u32
    {
        char buf[1024]{};
        format_to(buf,format,(Args&&)(args)...);
        return sys::write(buf);
    }
}