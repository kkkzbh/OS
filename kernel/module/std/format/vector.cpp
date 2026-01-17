

export module vector.format;

import format;
import vector;


template<typename T>
struct std::formatter<std::vector<T>>
{
    auto static parse(char*& out,vector<T> const& arg,char c) -> void
    {
        *out = '[';
        for(auto const& v : arg) {
            ++out;
            format<T>::parse(out,v,c);
            *out = ',';
        }
        *out++ = ']';
    }
};