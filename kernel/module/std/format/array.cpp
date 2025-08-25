

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
            using U = remove_const<T>;
            if constexpr (std::same_as<U,char> or std::same_as<U,char unsigned>) {
                format<char*>::parse(out,arg.data(),c);
            } else {
                *out = '[';
                for(auto const& v : arg) {
                    ++out;
                    format<T>::parse(out,v,c);
                    *out = ',';
                }
                *out++ = ']';
            }
        }

    };
}