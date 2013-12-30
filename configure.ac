dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(pris0nbarake, 1.0)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AM_PROG_CC_C_O
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS(config.h)

AC_PROG_CXX

ACCFLAGS="-I\$(top_srcdir)/include"
ACSHAREDLIBS=""
ACLDFLAGS=""

LIBXML2_STATIC=""
OPENSSL_STATIC=""

PKG_CHECK_MODULES(libimobiledevice, libimobiledevice-1.0 >= 1.1.0)
PKG_CHECK_MODULES(libplist, libplist >= 0.15)
PKG_CHECK_MODULES(libzip, libzip >= 0.8)

SYSTYPE=
case "$host_os" in
	darwin*)
		SYSTYPE=osx
		ACCFLAGS+=" -fPIC -arch i386 "
		ACSHAREDLIBS+=" -lpthread -lz -lm -liconv -lssl -lcrypto -lxml2  -framework MobileDevice -F/System/Library/PrivateFrameworks -lcurl"
		ACLDFLAGS+=" -arch i386 -lcurl -framework MobileDevice -F/System/Library/PrivateFrameworks "
		LIBSDIR="\$(top_srcdir)/staticlibs/$SYSTYPE"
		ACGUI_CFLAGS+="-DPIC -DHAVE_ASPRINTF -DHAVE_VASPRINTF \
			-DCURL_PULL_SYS_SOCKET_H \
			-D__WXOSX_COCOA__ -DwxDEBUG_LEVEL=0 \
			-I\$(top_srcdir)/include/wxWidgets-2.9.2/include -I\$(top_srcdir)/include/os/osx/wxWidgets-2.9.2/lib/wx/include/osx_cocoa-unicode-static-2.9"
		ACGUI_LDFLAGS+="-pthread -static-libstdc++"
		ACGUI_LDADD+="$LIBSDIR/libwxpng-2.9.a \
			$LIBSDIR/libwx_baseu-2.9.a \
			$LIBSDIR/libwx_osx_cocoau_core-2.9.a \
			-framework MobileDevice -F/System/Library/PrivateFrameworks -framework IOKit -framework Carbon -framework Cocoa -framework QuickTime -framework OpenGL -framework System -framework Security -liconv"
	;;
	linux*)
		case "$BUILD_HOST_CPU" in
		x86_64)
		SYSTYPE=linux.x86_64
		ACCFLAGS=" -fPIC -I\$(top_srcdir)/include -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES "
		ACSHAREDLIBS+=" -lpthread -lz -lm -lssl -lcrypto -lxml2"
		ACLDFLAGS+=" -s"
		LIBSDIR="\$(top_srcdir)/staticlibs/$SYSTYPE"
		ACGUI_CFLAGS+="-DPIC -DHAVE_ASPRINTF -DHAVE_VASPRINTF \
			-DwxDEBUG_LEVEL=0 \
			-I\$(top_srcdir)/include/wxWidgets-2.9.2/include \
			-I\$(top_srcdir)/include/os/linux/wxWidgets-2.9.2/lib/wx/include/x86_64-linux-gnu-gtk2-unicode-static-2.9 \
			-D__WXGTK__ -pthread"
                ACGUI_LDFLAGS+="-pthread -Wl,-Bsymbolic-functions -static-libstdc++"
                ACGUI_LDADD+="$LIBSDIR/libwx_gtk2u_core-2.9.a \
			$LIBSDIR/libwx_baseu-2.9.a \
			-lpthread -lz -lm -lglib-2.0 -lgtk-x11-2.0 -liconv"
		;;
		*)
		SYSTYPE=linux
		ACCFLAGS+=" -fPIC "
		ACSHAREDLIBS+=" -lpthread -lz -lm -lssl -lcrypto -lxml2"
		ACLDFLAGS+="-s"
		LIBSDIR="\$(top_srcdir)/staticlibs/$SYSTYPE"
		ACGUI_CFLAGS+="-DPIC -DHAVE_ASPRINTF -DHAVE_VASPRINTF \
			-DwxDEBUG_LEVEL=0 \
			-I\$(top_srcdir)/include/wxWidgets-2.9.2/include \
			-I\$(top_srcdir)/include/os/linux/wxWidgets-2.9.2/lib/wx/include/i686-linux-gnu-gtk2-unicode-static-2.9 \
			-D__WXGTK__ -pthread"
		ACGUI_LDFLAGS+="-pthread -Wl,-Bsymbolic-functions -L/lib32 -L/usr/lib32 -static-libstdc++"
		ACGUI_LDADD+="$LIBSDIR/libwx_gtk2u_core-2.9.a \
			$LIBSDIR/libwx_baseu-2.9.a \
			-lpthread -lz -lm -lglib-2.0 -lgtk-x11-2.0 -liconv"
		;;
		esac
	;;
	mingw32)
		SYSTYPE=win32
		ACCFLAGS+=" -DWIN32 -D__LITTLE_ENDIAN__=1 -DLIBXML_STATIC=1 "
		ACSHAREDLIBS+=" -lz -lm -lws2_32 -lgdi32 -lole32 "
		ACLDFLAGS+=" -s"
		LIBSDIR="\$(top_srcdir)/staticlibs/$SYSTYPE"
		ACGUI_CFLAGS+="-D_WIN32 -D__LITTLE_ENDIAN__=1 \
			-DCURL_PULL_WS2TCPIP_H=1 \
			-I\$(top_srcdir)/include/wxWidgets-2.8.12/include \
			-I\$(top_srcdir)/include/os/win32/wxWidgets-2.8.12/lib/wx/include/msw-unicode-release-static-2.8 \
			-D__WXMSW__ -DwxDEBUG_LEVEL=0 -mthreads"
		ACGUI_LDFLAGS+=" -mthreads -Wl,--subsystem,windows -mwindows -static-libstdc++"
		ACGUI_LDADD+="$LIBSDIR/libwx_mswu_core-2.8.a \
			$LIBSDIR/libwx_baseu-2.8.a \
			-lz -lm -lws2_32 -lgdi32 -lole32 -loleaut32 -luuid -lcomctl32 -lsetupapi"
		LIBXML2_STATIC="$LIBSDIR/libxml2.a"
		OPENSSL_STATIC="$LIBSDIR/libssl.a $LIBSDIR/libcrypto.a"
	;;
	*)
	;;
esac

if test "x$SYSTYPE" == "x"; then
	AC_MSG_ERROR([Unsupported system type $host_os])
fi

GLOBAL_CFLAGS=$ACCFLAGS
AC_SUBST(GLOBAL_CFLAGS)

LIBDIR="\$(top_srcdir)/staticlibs/$SYSTYPE"
GLOBAL_LIBS=" \
 $LIBDIR/libimobiledevice.a \
 $LIBDIR/libplist.a \
 $LIBDIR/libusbmuxd.a \
 $ACSHAREDLIBS \ 
"
AC_SUBST(GLOBAL_LIBS)

GLOBAL_LDFLAGS="$ACLDFLAGS"
AC_SUBST(GLOBAL_LDFLAGS)

GUI_CFLAGS=$ACGUI_CFLAGS
AC_SUBST(GUI_CFLAGS)

GUI_LDFLAGS=$ACGUI_LDFLAGS
AC_SUBST(GUI_LDFLAGS)

GUI_LDADD=$ACGUI_LDADD
AC_SUBST(GUI_LDADD)

AC_CONFIG_FILES(Makefile src/Makefile gui/Makefile)
AC_OUTPUT

