AC_PREREQ([2.69])
AC_INIT([stooge], [0.1], [clamberson@gmail.com])
AC_CONFIG_SRCDIR([src/stooge.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.15 foreign tar-pax subdir-objects -Wall -Werror])
AM_SILENT_RULES([yes])
AC_PROG_CC
PKG_CHECK_MODULES([LIBMICROHTTPD], [libmicrohttpd])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
