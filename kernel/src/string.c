

#include <string.h>
#include <global.h>
#include <assert.h>


void* memset(void* __dst,u8 value,size_t size)
{
    ASSERT(__dst != nullptr);
    auto dst = (char*)__dst;
    while(size--) {
        *dst++ = value;
    }
    return __dst;
}

void* memcpy(void* restrict __dst,void const* restrict __src, size_t size)
{
    ASSERT(__dst != nullptr && __src != nullptr);
    auto dst = (char*)__dst;
    auto src = (char const*)__src;
    while(size--) {
        *dst++ = *src++;
    }

    return __dst;
}

int memcmp(void const* __a,void const* __b, size_t size)
{
    ASSERT(__a != nullptr && __b != nullptr);
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

char* strcpy(char* restrict dst,char const* restrict src)
{
    ASSERT(dst != nullptr && src != nullptr);
    char* ret = dst;
    while((*dst++ = *src++)) {

    }
    return ret;
}

size_t strlen(char const* str)
{
    // 不计入 '\0'
    ASSERT(str != nullptr);
    char const* first = str;
    while(*str++) {

    }
    return str - first - 1;
}

int strcmp(char const* a,char const* b)
{
    ASSERT(a != nullptr && b != nullptr);
    while(*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a - *b;
}

char* strchr(char const* str,int ch)
{
    ASSERT(str != nullptr);
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

char* strrchr(char const* str,int ch)
{
    ASSERT(str != nullptr);
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

char* strcat(char* restrict dst,char const* restrict src)
{
    ASSERT(dst != nullptr && src != nullptr);
    char* str = dst;
    for(; *str; ++str) {

    }
    while((*str++ = *src++)) {

    }
    return dst;
}

size_t strchrs(char const* str,int ch)
{
    ASSERT(str != nullptr);
    size_t cnt = 0;
    char const* p = str;
    for(; *p; ++p) {
        if(*p == ch) {
            ++cnt;
        }
    }
    return cnt;
}