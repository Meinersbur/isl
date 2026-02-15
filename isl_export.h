
#ifndef ISL_EXPORT_H
#define ISL_EXPORT_H

#ifdef ISL_STATIC_DEFINE
#  define ISL_EXPORT
#  define ISL_NO_EXPORT
#else
#  ifndef ISL_EXPORT
#    ifdef isl_EXPORTS
        /* We are building this library */
#      define ISL_EXPORT 
#    else
        /* We are using this library */
#      define ISL_EXPORT 
#    endif
#  endif

#  ifndef ISL_NO_EXPORT
#    define ISL_NO_EXPORT 
#  endif
#endif

#ifndef ISL_DEPRECATED
#  define ISL_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ISL_DEPRECATED_EXPORT
#  define ISL_DEPRECATED_EXPORT ISL_EXPORT ISL_DEPRECATED
#endif

#ifndef ISL_DEPRECATED_NO_EXPORT
#  define ISL_DEPRECATED_NO_EXPORT ISL_NO_EXPORT ISL_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ISL_NO_DEPRECATED
#    define ISL_NO_DEPRECATED
#  endif
#endif

#endif /* ISL_EXPORT_H */
