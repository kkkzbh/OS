module;

#include <assert.h>
#include <string.h>

export module format;

import utility;

template<unsigned_integral T> // 无符号整形转字符串 成功转尾指针，失败行为未定义
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

template<typename T>
auto format(char* out,T&& arg) -> char* = delete("para for safe print");

export template<typename... Args>
auto format_to(char* out,char const* format,Args&&... args) -> u32
{
    auto base = out;
    void const* arg[sizeof...(args)]{ ((void const*)(&args))... };
    auto c = *format;
    auto i = 0;
    while(c) {
        if(c != '%') {
            *out++ = c;
            c = *++format;
            continue;
        }
        c = *++format;
        switch(c) {
            case 'x' : {
                auto v = *(u32*)arg[i++];
                to_chars(out,v,16);
                ASSERT(out != nullptr);
                c = *++format;
                break;
            } case 'c' : {
                *out++ = *(char*)arg[i++];
                c = *++format;
                break;
            } case 'd' : {
                auto v = *(int*)arg[i++];
                if(v < 0) {
                    v = -v;
                    *out++ = '-';
                }
                to_chars(out,(u32)v,10);
                c = *++format;
                break;
            } case 's' : {
                auto str = *(char**)arg[i++];
                strcpy(out,str);
                out += strlen(str);
                c = *++format;
                break;
            } default : {
                PANIC("format error!!");
            }
        }
    }
    return out - base;
}