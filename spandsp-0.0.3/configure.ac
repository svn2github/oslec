# configure.ac -- Process this file with autoconf to produce configure
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: configure.ac,v 1.17 2006/11/26 13:13:30 steveu Exp $

# @start 1

AC_INIT

AC_DEFUN([AX_COMPILER_VENDOR],
[
AC_CACHE_CHECK([for _AC_LANG compiler vendor], ax_cv_[]_AC_LANG_ABBREV[]_compiler_vendor,
 [ax_cv_[]_AC_LANG_ABBREV[]_compiler_vendor=unknown
  # note: don't check for GCC first, since some other compilers define __GNUC__
  for ventest in intel:__ICC,__ECC,__INTEL_COMPILER ibm:__xlc__,__xlC__,__IBMC__,__IBMCPP__ gnu:__GNUC__ sun:__SUNPRO_C,__SUNPRO_CC hp:__HP_cc,__HP_aCC dec:__DECC,__DECCXX,__DECC_VER,__DECCXX_VER borland:__BORLANDC__,__TURBOC__ comeau:__COMO__ cray:_CRAYC kai:__KCC lcc:__LCC__ metrowerks:__MWERKS__ sgi:__sgi,sgi microsoft:_MSC_VER watcom:__WATCOMC__ portland:__PGI; do
    vencpp="defined("`echo $ventest | cut -d: -f2 | sed 's/,/) || defined(/g'`")"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[
#if !($vencpp)
      thisisanerror;
#endif
])], [ax_cv_]_AC_LANG_ABBREV[_compiler_vendor=`echo $ventest | cut -d: -f1`; break])
  done
 ])
])

SPANDSP_MAJOR_VERSION=0
SPANDSP_MINOR_VERSION=0
SPANDSP_MICRO_VERSION=3

SPANDSP_LT_CURRENT=0
SPANDSP_LT_REVISION=2
SPANDSP_LT_AGE=0

VERSION=$SPANDSP_MAJOR_VERSION.$SPANDSP_MINOR_VERSION.$SPANDSP_MICRO_VERSION
PACKAGE=spandsp

AC_SUBST(SPANDSP_LT_CURRENT)
AC_SUBST(SPANDSP_LT_REVISION)
AC_SUBST(SPANDSP_LT_AGE)

AC_CONFIG_SRCDIR([src/tone_generate.c])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([src/config.h:config-h.in])
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
AC_PROG_GCC_TRADITIONAL
AC_PROG_LIBTOOL
AC_LANG([C])

AX_COMPILER_VENDOR

AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE

AC_CHECK_TYPES(long long)
AC_CHECK_TYPES(long double)

AC_TYPE_SIGNAL

AC_ARG_ENABLE(doc,     [  --enable-doc      Build the documentation])
AC_ARG_ENABLE(tests,   [  --enable-tests    Build the test programs])
AC_ARG_ENABLE(itutests,[  --enable-itutests Build TIFF test files for some ITU test images])
AC_ARG_ENABLE(mmx,     [  --enable-mmx      Enable MMX support])
AC_ARG_ENABLE(sse,     [  --enable-sse      Enable SSE support])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_VPRINTF
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES

AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([rint])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([strcasecmp])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([gettimeofday])

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

# Check for header files.
AC_CHECK_HEADERS([socket.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([tgmath.h])
AC_CHECK_HEADERS([math.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/fcntl.h])
AC_CHECK_HEADERS([audiofile.h])
AC_CHECK_HEADERS([fftw.h])
AC_CHECK_HEADERS([fftw3.h])
AC_CHECK_HEADERS([tiffio.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([unicall.h])
AC_CHECK_HEADERS([X11/X.h])

# Determine XML2 include path
AC_MSG_CHECKING(for libxml/xmlmemory.h)

# Can we include headers using system include dirs?
AC_TRY_COMPILE([#include <libxml/xmlmemory.h>], [int a = 1;],
    XML2_INCLUDE=" ",
    XML2_INCLUDE=
)

# Hunt through several possible directories to find the includes for libxml2
if test "x$XML2_INCLUDE" = "x"; then
    old_CPPFLAGS="$CPPFLAGS"
    for i in $xml2_include_dir /usr/include /usr/local/include /usr/include/libxml2 /usr/local/include/libxml2 ; do
        CPPFLAGS="$old_CPPFLAGS -I$i"
        AC_TRY_COMPILE([#include <libxml/xmlmemory.h>], [int a = 1;],
            XML2_INCLUDE="-I$i",
            XML2_INCLUDE=
        )
        if test "x$XML2_INCLUDE" != "x"; then
            break;
        fi
    done
    CPPFLAGS="$old_CPPFLAGS $XML2_INCLUDE"
fi

AC_CHECK_HEADERS([libxml/xmlmemory.h])
AC_CHECK_HEADERS([libxml/parser.h])
AC_CHECK_HEADERS([libxml/xinclude.h])

AC_LANG([C++])
AC_CHECK_HEADERS([FL/Fl.H])
AC_CHECK_HEADERS([FL/Fl_Overlay_Window.H])
AC_CHECK_HEADERS([FL/Fl_Light_Button.H])
AC_CHECK_HEADERS([FL/fl_draw.H])
AC_CHECK_HEADERS([FL/Fl_Cartesian.H], [], [], [],[[#include <FL/Fl.H>
]])
AC_CHECK_HEADERS([FL/Fl_Audio_Meter.H], [], [], [],[[#include <FL/Fl.H>
]])

AC_LANG([C])
# X86_64 Linux machines have both 64 bit and 32 bit libraries. We need to choose the right set
AC_CHECK_FILE([/usr/X11R6/lib64], [TESTLIBS="$TESTLIBS -L/usr/X11R6/lib64"], AC_CHECK_FILE([/usr/X11R6/lib], [TESTLIBS="$TESTLIBS -L/usr/X11R6/lib"]))

# Checks for libraries.
AC_CHECK_LIB([Xft], [XftFontOpen], TESTLIBS="$TESTLIBS -lXft",, $TESTLIBS)
AC_CHECK_LIB([Xext], [XextCreateExtension], TESTLIBS="$TESTLIBS -lXext",, $TESTLIBS)
AC_CHECK_LIB([X11], [XOpenDisplay], TESTLIBS="$TESTLIBS -lX11",, $TESTLIBS)
AC_CHECK_LIB([tiff], [TIFFOpen], , AC_MSG_ERROR("Can't build without libtiff (does your system require a libtiff-devel package?)"), -lm)
AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([xml2], [xmlParseFile], [AC_DEFINE([HAVE_LIBXML2], [1], [Define to 1 if you have the 'libxml2' library (-lxml2).]) TESTLIBS="$TESTLIBS -lxml2"])
if test -n "$enable_tests" ; then
    AC_LANG([C++])
    AC_CHECK_LIB([fltk], [main], TESTLIBS="$TESTLIBS -lfltk -lsupc++")
    AC_CHECK_LIB([fltk_cartesian], [main], TESTLIBS="-lfltk_cartesian $TESTLIBS")
    AC_CHECK_LIB([fltk_audio_meter], [main], TESTLIBS="-lfltk_audio_meter $TESTLIBS")
    AC_LANG([C])
    AC_CHECK_LIB([audiofile], [afOpenFile], TESTLIBS="$TESTLIBS -laudiofile", AC_MSG_ERROR("Can't make tests without libaudiofile (does your system require a libaudiofile-devel package?)"))
    AC_CHECK_LIB([fftw3], [fftw_plan_dft_1d], TESTLIBS="$TESTLIBS -lfftw3")
    AC_CHECK_LIB([fftw], [fftw_create_plan], TESTLIBS="$TESTLIBS -lfftw")
    AC_CHECK_LIB([pthread], [pthread_attr_init], TESTLIBS="$TESTLIBS -lpthread")
    AC_CHECK_LIB([dl], [dlopen], TESTLIBS="$TESTLIBS -ldl")
    AC_CHECK_LIB([unicall], [uc_start], TESTLIBS="$TESTLIBS -lunicall",, -ltiff -ldl)
fi

case "${ax_cv_c_compiler_vendor}" in
   gnu)
        COMP_VENDOR_CFLAGS="-O2 -g -std=gnu99 -Wall -Wunused-variable -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"
        if test "$enable_sse" = "yes" ; then
            COMP_VENDOR_CFLAGS="-msse $COMP_VENDOR_CFLAGS"
        fi
        if test "$enable_mmx" = "yes" ; then
            COMP_VENDOR_CFLAGS="-mmmx $COMP_VENDOR_CFLAGS"
        fi
        ;;
    *)
        COMP_VENDOR_CFLAGS="-O2 -g -std=c99 -Wall -Wunused-variable -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"
        ;;
esac

AM_CONDITIONAL([COND_DOC], [test "$enable_doc" = yes])
AM_CONDITIONAL([COND_TESTS], [test "$enable_tests" = yes])
AM_CONDITIONAL([COND_ITUTESTS], [test "$enable_itutests" = yes])
AM_CONDITIONAL([COND_MMX], [test "$enable_mmx" = yes])
AM_CONDITIONAL([COND_SSE], [test "$enable_sse" = yes])

AC_SUBST(TESTLIBS, $TESTLIBS)
AC_SUBST(COMP_VENDOR_CFLAGS, $COMP_VENDOR_CFLAGS)

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 itutests/Makefile
                 itutests/fax/Makefile
                 src/Makefile
                 tests/Makefile
                 spandsp.spec])

AC_OUTPUT

# @end 1
