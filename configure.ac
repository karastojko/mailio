#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mailio], [0.14.1], [contact@alepho.com])
LT_INIT
AC_CONFIG_SRCDIR([src/codec.cpp])
AC_CONFIG_HEADERS([config.h])

PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CXX
AM_INIT_AUTOMAKE([subdir-objects])

PKG_CHECK_MODULES([SSL], libssl)
PKG_CHECK_MODULES([CRYPTO], libcrypto)
BOOST_REQUIRE
BOOST_SYSTEM
BOOST_REGEX
BOOST_DATE_TIME

AC_CONFIG_FILES([Doxyfile])
AC_CONFIG_FILES([libmailio.pc])
AC_OUTPUT(Makefile src/Makefile examples/Makefile)
