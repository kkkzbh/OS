


#ifndef _KASSERT_H
#define _KASSERT_H

#include <cdefs.h>

__BEGIN_DECLS

void panic_spin(char const* filename, int line, char const* func,char const* condition);

#define PANIC(...) panic_spin(__FILE__,__LINE__,__func__,__VA_ARGS__)

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) \
        if(CONDITION) { \
            \
        } else {  \
            PANIC(#CONDITION);    \
        }
#endif // NDEBUG

__END_DECLS

#endif //_KASSERT_H
