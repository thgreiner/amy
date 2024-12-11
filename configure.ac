dnl configure.in - configure.in for Amy
dnl
dnl This file is part of Amy, a chess program by Thorsten Greiner
dnl
dnl Amy is copyrighted by Thorsten Greiner

AC_INIT([amy],[0.9.6-pre])
AC_CONFIG_SRCDIR(src/dbase.c)
AM_INIT_AUTOMAKE([foreign])

m4_include([m4/ax_gcc_builtin.m4])

AC_ARG_WITH(dbpath,
  [  --with-dbpath           specify path to rocksdb libraries])
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

AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h stdint.h stdatomic.h)

dnl Check for typedefs, structures and compiler characteristics
AC_TYPE_SIZE_T

AC_STRUCT_TM

AC_C_BIGENDIAN
AC_C_CONST

AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gettimeofday select strerror strstr setbuf gethostname ffsll)

AX_GCC_BUILTIN(__builtin_ctzll)
AX_GCC_BUILTIN(__builtin_popcountll)

AC_CONFIG_FILES([Makefile EPD/Makefile PGN/Makefile src/Makefile doc/Makefile])
AC_OUTPUT
