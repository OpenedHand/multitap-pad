AC_PREREQ(2.53)
AC_INIT(multitap-pad, 0.0, http://www.openedhand.com/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/multitap-pad-main.c)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(MTP, gtk+-2.0 >= 2.6.0 libfakekey matchbox-panel)

AC_OUTPUT([
Makefile
applet/Makefile
gtk-im/Makefile
src/Makefile
data/Makefile
])

