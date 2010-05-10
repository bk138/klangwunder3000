## Process this file with automake to produce Makefile.in



#SUBDIRS = 


AM_CPPFLAGS = \
	 -Wall \
	 -g \
	$(WX_CXXFLAGS) \
	$(libavformat_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(klangwunder3000_CFLAGS)

bin_PROGRAMS = klangwunder3000

klangwunder3000_SOURCES = \
	KW3KApp.cpp   \
	KW3KApp.h  \
	Klangset.cpp \
	Klangset.h \
	gui/FrameMain.cpp \
	gui/FrameMain.h \
	gui/MyFrameMain.cpp \
	gui/MyFrameMain.h \
	gui/bitmapFromMem.h\
	gui/res/about.png.h \
	gui/res/add.png.h \
	gui/res/info.png.h \
	gui/res/pause.png.h \
	gui/res/play.png.h \
	gui/res/remove.png.h \
	gui/res/stop.png.h \
	gui/res/klangwunder3000.xpm 

EXTRA_DIST = \
	gui/winres.rc\
	gui/res/icon.ico \
	gui/res/icon.icns \
	gui/res/about.png \
	gui/res/add.png \
	gui/res/info.png \
	gui/res/pause.png \
	gui/res/play.png \
	gui/res/remove.png \
	gui/res/stop.png 

klangwunder3000_LDFLAGS = 

klangwunder3000_LDADD = \
	$(libavformat_LIBS) \
	$(WX_LIBS) 



if MINGW
klangwunder3000_SOURCES += gui/winres.rc
.rc.o:
	 $(WX_RESCOMP) -o $@ $<
endif

