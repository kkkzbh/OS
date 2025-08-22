

export module string.format;

import format;
import string;

namespace std
{
    template<CharT Char>
    struct formatter<string_view<Char>>
    {
        auto static parse(char*& out,string_view<Char> arg,char c) -> void
        {
            format<char*>::parse(out,arg.data(),c);
        }
    };
}