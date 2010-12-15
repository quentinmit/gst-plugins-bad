
plugin_LTLIBRARIES = libgstsync.la

libgstsync_la_SOURCES = gstsync.c \
	gstvideosync.c \
	gstvideosync.h \
	gstaudiosync.c \
	gstaudiosync.h

libgstsync_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_CFLAGS)
libgstsync_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) \
		       $(GST_LIBS)
libgstsync_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsync_la_LIBTOOLFLAGS = --tag=disable-static


