AM_CFLAGS = -Wall

# TODO: Don't hardcode 2.10.0 binary version
imdir = @libdir@/gtk-2.0/2.10.0/immodules/
im_LTLIBRARIES = libmtp-im-invoker.la

multipress_defs = -DMULTIPRESS_LOCALEDIR=\""$(mplocaledir)"\" \
                  -DMULTIPRESS_CONFDIR=\""$(sysconfdir)/multitap-pad"\"

libmtp_im_invoker_la_CPPFLAGS = @MTP_CFLAGS@ -I${top_srcdir}/src \
                                $(multipress_defs)
libmtp_im_invoker_la_LDFLAGS = -avoid-version
libmtp_im_invoker_la_LIBADD = @MTP_LIBS@

libmtp_im_invoker_la_SOURCES = \
	im-context.h im-context.c \
	im-protocol.h im-protocol.c \
	gtkimcontextmultipress.c \
	gtkimcontextmultipress.h \
	immultipress.c
