AM_CPPFLAGS = \
 	-DLOCALEDIR=\"$(datadir)/locale\" 
AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = mwget

INCLUDES = @OPENSSL_CFLAGS@
mwget_LDADD = @OPENSSL_LIBS@

mwget_SOURCES = \
	macro.h \
	debug.h \
	debug.cpp \
	advio.h \
	advio.cpp \
	tcp.h \
	tcp.cpp \
	url.h \
	url.cpp \
	utils.h \
	utils.cpp \
	block.h \
	block.cpp \
	header.h \
	header.cpp \
	plugin.h \
	plugin.cpp \
	proxy.h \
	proxy.cpp \
	task.h \
	task.cpp \
	http.h \
	http.cpp \
	httpplugin.h \
	httpplugin.cpp \
	ftp.h \
	ftp.cpp \
	ftpparser.h \
	ftpparser.cpp \
	ftpplugin.h \
	ftpplugin.cpp \
	downloader.h \
	downloader.cpp \
	mwget.h \
	mwget.cpp \
	progressbar.h \
	progressbar.cpp
