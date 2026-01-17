

#include <string.h>
#include <global.h>
#include <assert.h>
#include <string_impl.h>


void* memset(void* __dst,u8 value,size_t size)
{
    ASSERT(__dst != nullptr);
    return memset_impl(__dst,value,size);
}

void* memcpy(void* restrict __dst,void const* restrict __src, size_t size)
{
    ASSERT(__dst != nullptr && __src != nullptr);
    return memcpy_impl(__dst,__src,size);
}

int memcmp(void const* __a,void const* __b, size_t size)
{
    ASSERT(__a != nullptr && __b != nullptr);
    return memcmp_impl(__a,__b,size);
}

char* strcpy(char* restrict dst,char const* restrict src)
{
    ASSERT(dst != nullptr && src != nullptr);
    return strcpy_impl(dst,src);
}

size_t strlen(char const* str)
{
    // 不计入 '\0'
    ASSERT(str != nullptr);
    return strlen_impl(str);
}

int strcmp(char const* a,char const* b)
{
    ASSERT(a != nullptr && b != nullptr);
    return strcmp_impl(a,b);
}

char* strchr(char const* str,int ch)
{
    ASSERT(str != nullptr);
    return strchr_impl(str,ch);
}

char* strrchr(char const* str,int ch)
{
    ASSERT(str != nullptr);
    return strrchr_impl(str,ch);
}

char* strcat(char* restrict dst,char const* restrict src)
{
    ASSERT(dst != nullptr && src != nullptr);
    return strcat_impl(dst,src);
}

size_t strchrs(char const* str,int ch)
{
    ASSERT(str != nullptr);
    return strchrs_impl(str,ch);
}