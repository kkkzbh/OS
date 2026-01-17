export module format;

import utility;
import string;

namespace std
{
    template<std::unsigned_integral T> // 无符号整形转字符串 成功转尾指针，失败行为未定义
    auto to_chars(char* &first,T val,int base = 10) -> void
    {
        auto lv = T(val % base);
        auto hv = T(val / base);
        if(hv) {
            to_chars(first,hv,base);
        }
        *first++ = [=] {
            if(lv < 10) {
                return lv + '0';
            }
            return lv - 10 + 'A';
        }();
    }

    export template<typename T>
    struct formatter
    {
        auto static parse(char*& out,T const& arg,char c) -> void;
    };

    export template<typename T>
    using format = formatter<std::normalize<T>>;


    template<>
    struct formatter<char>
    {
        auto static parse(char*& out,char arg,char) -> void
        {
            *out++ = arg;
        }
    };

    template<>
    struct formatter<unsigned char>
    {
        auto static parse(char*& out,char arg,char c) -> void
        {
            formatter<char>::parse(out,arg,c);
        }
    };

    template<>
    struct formatter<char*>
    {
        auto static parse(char*& out,char const* arg,char c) -> void
        {
            strcpy(out,arg);
            out += strlen(arg);
        }
    };

    template<size_t N>
    struct formatter<char[N]>
    {
        auto static parse(char*& out,char const* arg,char c) -> void
        {
            formatter<char*>::parse(out,arg,c);
        }
    };

    template<size_t N>
    struct formatter<unsigned char[N]>
    {
        auto static parse(char*& out,unsigned char const* arg,char c) -> void
        {
            formatter<char*>::parse(out,reinterpret_cast<char const*>(arg),c);
        }
    };

    template<std::unsigned_integral T>
    struct formatter<T>
    {
        auto static parse(char*& out,T arg,char c) -> void
        {
            switch(c) {
                case 'c': {
                    format<char>::parse(out,arg,c);
                    break;
                }
                case 'x': {
                    to_chars(out,arg,16);
                    break;
                } default: case 'd': {
                    to_chars(out,arg,10);
                    break;
                }
            }
        }
    };

    template<std::signed_integral T>
    struct formatter<T>
    {
        auto static parse(char*& out,T arg,char c) -> void
        {
            if(arg < 0) {
                *out++ = '-';
                arg = -arg;
            }
            using unsigned_type = std::make_unsigned<T>;
            formatter<unsigned_type>::parse(out,(unsigned_type)arg,c);
        }
    };

    template<size_t I = 0, typename... Args>
    auto parse(char*& out, size_t idx, char c, Args&&... args) -> void
    {
        if constexpr (I < sizeof...(Args)) {
            if (I == idx) {
                format<Args...[I]>::parse(out, args...[I], c);
            } else {
                parse<I + 1>(out, idx, c, static_cast<Args&&>(args)...);
            }
        }
    }

    export template<typename... Args>
    auto format_to(char* out, char const* fmt, Args&&... args) -> u32
    {
        auto base = out;
        auto c = *fmt;
        auto i = size_t{0};
        while(c) {
            if(c != '{') {
                *out++ = c;
                c = *++fmt;
                continue;
            }
            c = *++fmt;
            parse(out, i++, c, static_cast<Args&&>(args)...);
            c = *(fmt += 1 + (c != '}'));
        }
        return out - base;
    }
}
