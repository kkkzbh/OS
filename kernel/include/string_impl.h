
#ifndef STRING_IMPL_H
#define STRING_IMPL_H

#include <global.h>

void* memset_impl(void* __dst,u8 value,size_t size);
void* memcpy_impl(void* restrict __dst,void const* restrict __src, size_t size);
int memcmp_impl(void const* __a,void const* __b, size_t size);
char* strcpy_impl(char* restrict dst,char const* restrict src);
size_t strlen_impl(char const* str);
int strcmp_impl(char const* a,char const* b);
char* strchr_impl(char const* str,int ch);
char* strrchr_impl(char const* str,int ch);
char* strcat_impl(char* restrict dst,char const* restrict src);
size_t strchrs_impl(char const* str,int ch);

#endif
