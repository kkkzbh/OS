

export module string.format;

import format;
import string;


template<std::CharT Char>
struct std::formatter<std::string_view<Char>>
{
    auto static parse(char*& out,string_view<Char> arg,char c) -> void
    {
        format<char*>::parse(out,arg.data(),c);
    }
};
