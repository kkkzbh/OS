

#include <global.h>

void* memset_impl(void* __dst,u8 value,size_t size)
{
    auto dst = (char*)__dst;
    while(size--) {
        *dst++ = value;
    }
    return __dst;
}

void* memcpy_impl(void* restrict __dst,void const* restrict __src, size_t size)
{
    auto dst = (char*)__dst;
    auto src = (char const*)__src;
    while(size--) {
        *dst++ = *src++;
    }

    return __dst;
}

int memcmp_impl(void const* __a,void const* __b, size_t size)
{
    auto a = (char const*)__a;
    auto b = (char const*)__b;
    while(size--) {
        auto const cmp = *a++ - *b++;
        if(cmp) {
            return cmp;
        }
    }
    return 0;
}

char* strcpy_impl(char* restrict dst,char const* restrict src)
{
    char* ret = dst;
    while((*dst++ = *src++)) {

    }
    return ret;
}

size_t strlen_impl(char const* str)
{
    // 不计入 '\0'
    char const* first = str;
    while(*str++) {

    }
    return str - first - 1;
}

int strcmp_impl(char const* a,char const* b)
{
    while(*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a - *b;
}

char* strchr_impl(char const* str,int ch)
{
    for(; *str; ++str) {
        if(*str == ch) {
            return (char*)str;
        }
    }
    if(ch == '\0') {
        return (char*)str;
    }
    return nullptr;
}

char* strrchr_impl(char const* str,int ch)
{
    char const* ret = nullptr;
    for(; *str; ++str) {
        if(*str == ch) {
            ret = str;
        }
    }
    if(ch == '\0') {
        return (char*)str;
    }
    return (char*)ret;
}

char* strcat_impl(char* restrict dst,char const* restrict src)
{
    char* str = dst;
    for(; *str; ++str) {

    }
    while((*str++ = *src++)) {

    }
    return dst;
}

size_t strchrs_impl(char const* str,int ch)
{
    size_t cnt = 0;
    char const* p = str;
    for(; *p; ++p) {
        if(*p == ch) {
            ++cnt;
        }
    }
    return cnt;
}
