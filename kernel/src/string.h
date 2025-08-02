

#ifndef _KSTRING_H
#define _KSTRING_H

#include <cdefs.h>

__BEGIN_DECLS

#ifdef __cplusplus
    #define __RESTRICT __restrict__
#else
    #define __RESTRICT restrict
#endif

#include <stdint.h>

void* memset(void* __dst,u8 value,size_t size);

void* memcpy(void* __RESTRICT __dst,void const* __RESTRICT __src, size_t size);

int memcmp(void const* __a,void const* __b, size_t size);

char* strcpy(char* __RESTRICT dst,char const* __RESTRICT src);

size_t strlen(char const* str);

int strcmp(char const* a,char const* b);

char* strchr(char const* str,int ch);

char* strrchr(char const* str,int ch);

char* strcat(char* __RESTRICT dst,char const* __RESTRICT src);

size_t strchrs(char const* str,int ch);

__END_DECLS

#endif //_KSTRING_H
