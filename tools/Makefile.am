AM_CFLAGS = @LIBPNG_CFLAGS@ @LIBWEBP_CFLAGS@ @LIBLZMA_CFLAGS@ \
	@LIBLZ4_CFLAGS@ @LIBZSTD_CFLAGS@ @LIBOPENJP2_CFLAGS@ \
	@LIBCURL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBFONTCONFIG_CFLAGS@ \
	@LIBFREETYPE2_CFLAGS@ @LIBPIXMAN_CFLAGS@ @LIBCAIRO_CFLAGS@ \
	@LIBSPATIALITE_CFLAGS@
AM_LDFLAGS = -L../src 

bin_PROGRAMS = rl2sniff rl2tool wmslite

AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/headers

rl2sniff_SOURCES = rl2sniff.c

rl2tool_SOURCES = rl2tool.c

wmslite_SOURCES = wmslite.h wmslitecgi.c wmslite_config.c \
	wmslite_miniserver.c wmslite_sql.c wmslite_capabilities.c \
	wmslite_common.c

rl2sniff_LDADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@ \
	-lrasterlite2 -lsqlite3 -lpthread -lm
	
rl2tool_LDADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@ \
	-lrasterlite2 -lsqlite3 -lpthread -lm

if MINGW
wmslite_LDADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@ \
	-lrasterlite2 -lsqlite3 -lpthread -lm -lws2_32
else
wmslite_LDADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@ \
	-lrasterlite2 -lsqlite3 -lpthread -lm 
endif
