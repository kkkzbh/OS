

export module array.format;

import array;
import format;
import utility;

namespace std
{
    template<typename T,size_t N>
    struct formatter<array<T,N>>
    {
        auto static parse(char*& out,array<T,N> const& arg,char c) -> void
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
}