#ifndef HAVE___ATTRIBUTE__
#define __attribute__(x)
#endif

#if HAVE_DECL_FFS
#include <strings.h>
#endif

#if (HAVE_DECL_FFS==0) && (HAVE_DECL___BUILTIN_FFS==1)
#define ffs __builtin_ffs
#endif

#if !HAVE_DECL_FFS && !HAVE_DECL___BUILTIN_FFS && HAVE_DECL__BITSCANFORWARD
int isl_ffs(int i);
#define ffs isl_ffs
#endif

#ifdef GCC_WARN_UNUSED_RESULT
#define WARN_UNUSED	GCC_WARN_UNUSED_RESULT
#else
#define WARN_UNUSED
#endif
