

#ifndef _CDEFS_H
#define _CDEFS_H

/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef	__cplusplus
#define __BEGIN_DECLS	extern "C" {
#define __END_DECLS	}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#ifdef __cplusplus
#define __CONSTEXPR constexpr
#else
#define __CONSTEXPR
#endif

#endif //_CDEFS_H
