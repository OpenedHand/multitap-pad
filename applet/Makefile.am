appletdir = $(libdir)/matchbox-panel
applet_LTLIBRARIES = libmultitap-pad.la

libmultitap_pad_la_SOURCES = applet.c $(top_srcdir)/gtk-im/im-protocol.c
libmultitap_pad_la_CPPFLAGS = @MTP_CFLAGS@ -I$(top_srcdir) \
                              -I${top_srcdir}/src
libmultitap_pad_la_LDFLAGS = -avoid-version
libmultitap_pad_la_LIBADD = @MTP_LIBS@
