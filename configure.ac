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

PKG_CHECK_MODULES(MTP, gtk+-2.0 >= 2.6.0 libfakekey)


BUILD_APPLET=
AC_ARG_ENABLE(applet, AC_HELP_STRING([--enable-applet],[Build matchbox-panel
applet [[default=yes]]]), [want_applet="$enableval"], [want_applet=yes])
if test "x$want_applet" = "xyes"; then
  PKG_CHECK_MODULES(MBP, matchbox-panel)
  BUILD_APPLET=applet
fi
AC_SUBST(BUILD_APPLET)

AC_OUTPUT([
Makefile
applet/Makefile
gtk-im/Makefile
src/Makefile
data/Makefile
])

