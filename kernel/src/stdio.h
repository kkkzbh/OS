

#ifndef _KSTDIO_H
#define _KSTDIO_H

#include <cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

void putchar(int c);

void puts(char const* str);

void puthex(u32 num);

void clear();

__END_DECLS

#endif
