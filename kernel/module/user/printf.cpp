

export module printf;

import format;
import utility;
import sys;
import file.structure;


namespace std
{
    export template<typename... Args>
    auto printf(char const* format,Args&&... args) -> u32
    {
        char buf[1024]{};
        format_to(buf,format,forward<Args>(args)...);
        return std::write(stdout,buf,std::size(buf)).value_or(-1);
    }

    export template<typename... Args>
    auto sprintf(char* buf,char const* format,Args&&... args) -> u32
    {
        return format_to(buf,format,forward<Args>(args)...);
    }
}