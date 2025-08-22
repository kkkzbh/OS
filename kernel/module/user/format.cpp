module;

#include <assert.h>
#include <string.h>

export module format;

import utility;

namespace std
{
    template<std::unsigned_integral T> // 无符号整形转字符串 成功转尾指针，失败行为未定义
    auto to_chars(char* &first,T val,int base = 10) -> void
    {
        auto lv = val % base;
        auto hv = val / base;
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

    template<typename... Args>
    auto switch_table(char*& out,Args&&... args) -> auto
    {
        return [args...,&out](size_t idx,char c) {
            using seq = std::index_sequence_for<Args...>;
            return [=,&out]<size_t... Is>(std::index_sequence<Is...>) {
            ([&]{ if(Is == idx) format<Args>::parse(out,args...[Is],c); }(),...);
            }(seq{});
        };
    }

    export template<typename... Args>
    auto format_to(char* out,char const* format,Args&&... args) -> u32
    {
        auto base = out;
        auto parse = switch_table(out,std::forward<Args>(args)...);
        auto c = *format;
        auto i = 0;
        while(c) {
            if(c != '{') {
                *out++ = c;
                c = *++format;
                continue;
            }
            c = *++format;
            parse(i++,c);
            c = *(format += 1 + (c != '}'));
        }
        return out - base;
    }
}
