dnl configure.in - configure.in for Amy
dnl
dnl This file is part of Amy, a chess program by Thorsten Greiner
dnl
dnl Amy is copyrighted by Thorsten Greiner

AC_INIT([amy],[0.9.8-pre])
AC_CONFIG_SRCDIR(src/dbase.c)
AM_INIT_AUTOMAKE([foreign])

# NB: source lists in Makefile.am files must stay literal file names.
# automake parses _SOURCES values to build the dist/object lists and
# mangles make functions ($(wildcard)/$(filter)) inside them.  The
# headers in include/Makefile.am can use a wildcard because noinst_HEADERS
# is only expanded by make at dist time.

AC_CANONICAL_HOST

m4_include([m4/ax_gcc_builtin.m4])

AC_ARG_ENABLE(mt,
  [  --enable-mt             enable multithreaded search])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC()
AC_PROG_CXX()

mp_val="0"
if test "X$enable_mt" = "Xyes" ; then
dnl Check for pthreads
mp_val="1"
AC_CHECK_LIB(pthread, pthread_create)

dnl Check for pthreads in libc_r, this is for FreeBSD 5.2 and below
if test $ac_cv_lib_pthread_pthread_create != yes; then
AC_CHECK_LIB(c_r, pthread_create)
fi

fi
AC_DEFINE_UNQUOTED(MP, $mp_val, Enable multithreaded search)

AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h stdint.h stdatomic.h termios.h)

dnl Check for typedefs, structures and compiler characteristics
AC_TYPE_SIZE_T

AC_STRUCT_TM

AC_C_BIGENDIAN
AC_C_CONST

AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gettimeofday select strerror strstr setbuf gethostname ffsll getline tcgetattr)

if test "x$ac_cv_header_termios_h" = "xyes" -a "x$ac_cv_func_tcgetattr" = "xyes"; then
  AC_DEFINE([HAVE_LINENOISE], [1],
            [Define to 1 to use linenoise for interactive line editing])
fi
AM_CONDITIONAL([HAVE_LINENOISE],
  [test "x$ac_cv_header_termios_h" = "xyes" -a "x$ac_cv_func_tcgetattr" = "xyes"])

dnl The hand-rolled ctypes nn_extract dylib (src/Makefile.am) links the
dnl plain, non-fPIC objects of the Amy binary.  That only works on macOS
dnl Apple Silicon, where objects are position-independent by default.
AC_MSG_CHECKING(whether to build the nn_extract ctypes library)
case "$host" in
  aarch64-apple-darwin*)
    build_nn_extract=yes ;;
  *)
    build_nn_extract=no ;;
esac
AC_MSG_RESULT($build_nn_extract)
AM_CONDITIONAL([BUILD_NN_EXTRACT], [test "x$build_nn_extract" = xyes])

AX_GCC_BUILTIN(__builtin_ctzll)
AX_GCC_BUILTIN(__builtin_popcountll)

AC_CONFIG_FILES([Makefile EPD/Makefile PGN/Makefile src/Makefile doc/Makefile include/Makefile])
AC_OUTPUT
