

export module vector.format;

import format;
import vector;

namespace std
{
    template<typename T>
    struct formatter<vector<T>>
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
}