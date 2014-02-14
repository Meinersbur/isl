AC_DEFUN([AX_DETECT_GMP], [
AX_SUBMODULE(gmp,system|build,system)
case "$with_gmp" in
system)
	if test "x$with_gmp_prefix" != "x"; then
		isl_configure_args="$isl_configure_args --with-gmp=$with_gmp_prefix"
		GMP_CPPFLAGS="-I$with_gmp_prefix/include"
		GMP_LDFLAGS="-L$with_gmp_prefix/lib"
	fi
	GMP_LIBS=-lgmp
	SAVE_CPPFLAGS="$CPPFLAGS"
	SAVE_LDFLAGS="$LDFLAGS"
	SAVE_LIBS="$LIBS"
	CPPFLAGS="$GMP_CPPFLAGS $CPPFLAGS"
	LDFLAGS="$GMP_LDFLAGS $LDFLAGS"
	LIBS="$GMP_LIBS $LIBS"
	AC_CHECK_HEADER([gmp.h], [], [AC_ERROR([gmp.h header not found])])
	AC_CHECK_LIB([gmp], [main], [], [AC_ERROR([gmp library not found])])
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <gmp.h>]], [[
		mpz_t n, d;
		if (mpz_divisible_p(n, d))
			mpz_divexact_ui(n, n, 4);
	]])], [], [AC_ERROR([gmp library too old])])
	CPPFLAGS="$SAVE_CPPFLAGS"
	LDFLAGS="$SAVE_LDFLAGS"
	LIBS="$SAVE_LIBS"
	;;
build)
	GMP_CPPFLAGS="-I$gmp_srcdir -I$with_gmp_builddir"
	GMP_LIBS="$with_gmp_builddir/libgmp.la"
	;;
esac
SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
SAVE_LIBS="$LIBS"
CPPFLAGS="$GMP_CPPFLAGS $CPPFLAGS"
LDFLAGS="$GMP_LDFLAGS $LDFLAGS"
LIBS="$GMP_LIBS $LIBS"
need_get_memory_functions=false
AC_CHECK_DECLS(mp_get_memory_functions,[],[
	need_get_memory_functions=true
],[#include <gmp.h>])
CPPFLAGS="$SAVE_CPPFLAGS"
LDFLAGS="$SAVE_LDFLAGS"
LIBS="$SAVE_LIBS"
AM_CONDITIONAL(NEED_GET_MEMORY_FUNCTIONS, test x$need_get_memory_functions = xtrue)
])
