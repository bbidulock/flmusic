#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#AC_PREREQ([2.65])
AC_INIT([flmusic-fltk13], [1.2.1], [szukw000@arcor.de])
AC_CONFIG_SRCDIR([player.c])
AC_CONFIG_HEADERS([config.h])
# Checks for programs.
AC_PROG_CXX
dnl
dnl Check for FLTK-1.3
dnl
AC_PATH_PROG(FLTKCONFIG,fltk-config)
if test "x$FLTKCONFIG" = x; then
    AC_MSG_ERROR([   ==> Panic: fltk-config not found.])
else
    CXXFLAGS="`$FLTKCONFIG --cxxflags` $CXXFLAGS"
    LIBS="`$FLTKCONFIG --ldflags` $LIBS"
fi
dnl
dnl Check for PTHREAD
dnl
threadlibs=""
AC_CHECK_LIB(pthread, pthread_create, threadlibs=-lpthread)
if [[ -n "$threadlibs" ]] ; then
	AC_DEFINE(HAVE_LIBPTHREAD, [1], [define to 1 if you have libpthread])
else
	AC_MSG_ERROR([   ==> Panic: pthread not found.])
fi
dnl
dnl Check for ALSA
dnl
AC_PATH_PROG(PKGCONFIG,pkg-config)
if test "x$PKGCONFIG" = x; then
    AC_MSG_RESULT([   ==> pkg-config not found. Assume -lasound])
    LIBS="-lasound $LIBS"
else
    LIBS="`$PKGCONFIG --libs alsa` $LIBS"
fi
dnl
dnl Test for SOX
dnl
with_soxlibs="no"
soxlibs=""
AC_CHECK_LIB(sox, sox_open_read, soxlibs=-lsox)
if [[ -n "$soxlibs" ]] ; then
 with_soxlibs="yes"
 AC_DEFINE(HAVE_LIBSOX, [1], [define to 1 if you have libsox])
fi
AC_SUBST(soxlibs)
AC_SUBST(with_soxlibs)
dnl
dnl Test for LANG
dnl
AC_MSG_CHECKING(whether to use language dependent texts)
WithLANG="english"
AC_ARG_WITH(language,
[  --with-language[=[english|german]]  Build with language support],
test "$with_language" = "german" && WithLANG="german"
test "$with_language" = "english"  && WithLANG="english")
dnl Template:
dnl test "$with_language" = "french"  && WithLANG="french")
dnl
AC_MSG_RESULT($WithLANG)
if test "x$WithLANG" = "xenglish" ; then
  AC_DEFINE(WITH_ENGLISH, [1], [define to 1 if you prefer english texts])
fi
if test "x$WithLANG" = "xgerman" ; then
  AC_DEFINE(WITH_GERMAN, [1], [define to 1 if you prefer german texts])
fi
dnl Template:
dnl if test "x$WithLANG" = "xfrench" ; then
dnl  AC_DEFINE(WITH_FRENCH, [1], [define to 1 if you prefer french texts])
dnl fi
# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h sys/stat.h])
dnl
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
dnl
# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strdup strerror strrchr])
dnl
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
