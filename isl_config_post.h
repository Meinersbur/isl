#ifndef HAVE___ATTRIBUTE__
#define __attribute__(x)
#endif

#if (HAVE_DECL_FFS==0) && (HAVE_DECL___BUILTIN_FFS==1)
#define ffs __builtin_ffs
#endif
