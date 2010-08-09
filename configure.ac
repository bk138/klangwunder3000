dnl Process this file with autoconf to produce a configure script.

AC_INIT(klangwunder3000, 0.1, http://sourceforge.net/projects/klangwunder3000)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)



AC_CANONICAL_HOST
case $host_os in
     darwin*)
	HOST_IS_DARWIN="yes"
	;;
     *mingw32*)
	HOST_IS_WINDOWS="yes"
	;;
     *)
	HOST_IS_DARWIN="no"
	HOST_IS_WINDOWS="no"
	;;
esac


AM_CONDITIONAL(MINGW, [ test "$HOST_IS_WINDOWS" = "yes" ])
AM_CONDITIONAL(DARWIN, [ test "$HOST_IS_DARWIN" = "yes" ])


dnl default for wx-config
WXCONFIG="wx-config"
AC_ARG_WITH(wx-config, 
[[  --with-wx-config=FILE
    Use the given path to wx-config when determining
    wxWidgets configuration; defaults to "wx-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        WXCONFIG=$withval
    fi
])



dnl Check if we have enabled debug support.
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug          enable debug build [[default=no]]],, enable_debug="no")
if test "x$enable_debug" = "xyes"; then
	CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused-parameter"
	CFLAGS="$CFLAGS -DMULTICAST_DEBUG"
	WXCONFIG="$WXCONFIG --debug=yes"	
	AC_MSG_RESULT(yes)
else
	WXCONFIG="$WXCONFIG --debug=no"	
	CFLAGS="$CFLAGS -O3"	
	CXXFLAGS="$CXXFLAGS -Wall -O3 -fno-strict-aliasing "	
	AC_MSG_RESULT(no)
fi



AC_MSG_CHECKING([wxWidgets version])
if wxversion=`$WXCONFIG --version`; then
    AC_MSG_RESULT([$wxversion])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([wxWidgets is required. Try --with-wx-config.])
fi


dnl check for universal wx lib
if test "$HOST_IS_DARWIN" = "yes"; then
   AC_MSG_CHECKING([whether wxWidgets is a universal lib])
   if test ! -z "`$WXCONFIG --libs | grep "arch i386" 2>/dev/null`" \
        -a ! -z "`$WXCONFIG --libs | grep "arch ppc" 2>/dev/null`"; then
     CC="gcc -arch i386 -arch ppc"
     CXX="g++ -arch i386 -arch ppc"
     CPP="cpp"
     CXXCPP="cpp"
     AC_MSG_RESULT([yes])
   else
     AC_MSG_RESULT([no])
   fi
fi


# check libs for reading and playing audio
PKG_CHECK_MODULES(AVFORMAT, libavformat)
PKG_CHECK_MODULES(ALUT, freealut)


dnl gdb under windows does backtraces only with stabs format?
if test "$HOST_IS_WINDOWS" = "yes"; then
   CFLAGS="$CFLAGS -gstabs+"
   CXXFLAGS="$CXXFLAGS -gstabs+"
fi
 

WX_CPPFLAGS="`$WXCONFIG --cppflags`"
WX_CXXFLAGS="`$WXCONFIG --cxxflags`"
WX_LIBS="`$WXCONFIG --libs`"
WX_RESCOMP="`$WXCONFIG --rescomp`"

 
AC_SUBST(WX_CPPFLAGS)
AC_SUBST(WX_CXXFLAGS)
AC_SUBST(WX_LIBS)
AC_SUBST(WX_RESCOMP)
AC_SUBST(libavformat_CFLAGS)
AC_SUBST(libavformat_LIBS)


AC_ISC_POSIX
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_HEADER_STDC


AC_SUBST(klangwunder3000_CFLAGS)
AC_SUBST(klangwunder3000_LIBS)



dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
#GETTEXT_PACKAGE=klangwunder3000
#AC_SUBST(GETTEXT_PACKAGE)
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
#IT_PROG_INTLTOOL([0.35.0])




# ***************************************************************************
# finally
# ***************************************************************************
AC_OUTPUT([
Makefile
src/Makefile
])
