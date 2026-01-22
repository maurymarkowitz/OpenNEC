#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nec2c], [1.3], [nkcyham@yahoo.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

# Set CFLAGS if not set in environment
AC_MSG_CHECKING([whether configure should try to set CFLAGS])
if test "x${CFLAGS+set}" = xset; then
   enable_cflags_setting=no
else
   enable_cflags_setting=yes
fi
AC_MSG_RESULT($enable_cflags_setting)
test "x$enable_cflags_setting" = xyes && CFLAGS="-Wall -O2"

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([m], [sin])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor pow sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
