## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

klangwunder3000docdir = ${datadir}/doc/klangwunder3000
klangwunder3000doc_DATA = \
	README\
	AUTHORS\
	ChangeLog\
	NEWS

Applicationsdir = $(datadir)/applications/
Applications_DATA = src/klangwunder3000.desktop

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = src/gui/res/klangwunder3000.xpm


EXTRA_DIST = $(klangwunder3000doc_DATA) \
             $(Applications_DATA) \
             $(pixmap_DATA)\
             debian/changelog  debian/compat  debian/control  debian/copyright\
             debian/dirs  debian/docs  debian/menu  debian/rules  debian/watch


if DARWIN
bundle_contents = Klangwunder3000.app/Contents
appbundle: src/klangwunder3000
	mkdir -p $(bundle_contents)/MacOS
	mkdir -p $(bundle_contents)/Resources
	echo "APPL????" > $(bundle_contents)/PkgInfo
	$(INSTALL_PROGRAM) $< $(bundle_contents)/MacOS/
	cp src/gui/res/icon.icns $(bundle_contents)/Resources
	cp README  $(bundle_contents)/Resources
	cp NEWS    $(bundle_contents)/Resources
	cp COPYING $(bundle_contents)/Resources
	cp AUTHORS $(bundle_contents)/Resources
	echo \
	"<?xml version="1.0" encoding="UTF-8"?>\
	<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"\
	<plist version="1.0">\
	<dict>\
	<key>CFBundleDevelopmentRegion</key>\
	<string>English</string>\
	<key>CFBundleExecutable</key>\
	<string>@PACKAGE_NAME@</string>\
	<key>CFBundleIconFile</key>\
	<string>icon.icns</string>\
	<key>CFBundleGetInfoString</key>\
	<string>Klangwunder3000 @PACKAGE_VERSION@, © Christian Beier (dontmind@freeshell.org) 2007-2010 </string>\
	<key>CFBundleName</key>  \
	<string>Klangwunder3000</string> \
	<key>CFBundlePackageType</key> \
	<string>APPL</string>          \
	<key>CFBundleShortVersionString</key> \
	<string>@PACKAGE_VERSION@</string> \
	<key>CFBundleVersion</key> \
	<string>@PACKAGE_VERSION@</string> \
	</dict>   \
	</plist> \
	" >  $(bundle_contents)/Info.plist
endif


if MINGW
nsis_installer: all klangwunder3000.nsi README NEWS COPYING
	cat README | unix2dos > README.TXT
	cat NEWS | unix2dos > NEWS.TXT
	cat COPYING | unix2dos > COPYING.TXT
	$(STRIP) src/klangwunder3000.exe
	makensis klangwunder3000.nsi 
endif


