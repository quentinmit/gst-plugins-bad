plugin_LTLIBRARIES = libgststreamanalysis.la

libgststreamanalysis_la_SOURCES = gststreamanalysis.c
libgststreamanalysis_la_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_CFLAGS)
libgststreamanalysis_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS)
libgststreamanalysis_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgststreamanalysis_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gststreamanalysis.h


