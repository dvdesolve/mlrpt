#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mlrpt], [1.6.4], [nkcyham@yahoo.com])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/Mlrpt/main.c])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

# Set CFLAGS if not set in environment
AC_MSG_CHECKING([whether configure should try to set CFLAGS])
if test "x${CFLAGS+set}" = xset; then
   enable_cflags_setting=no
else
   enable_cflags_setting=yes
fi
AC_MSG_RESULT($enable_cflags_setting)
test "x$enable_cflags_setting" = xyes && CFLAGS=""

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([rtlsdr], [main])
AC_CHECK_LIB([airspy], [main])

# Coditional compilation of sources
AM_CONDITIONAL([USE_LIBRTLSDR],[grep HAVE_LIBRTLSDR confdefs.h > /dev/null])
AM_CONDITIONAL([USE_LIBAIRSPY],[grep HAVE_LIBAIRSPY confdefs.h > /dev/null])

# Internationalization tools
AM_GNU_GETTEXT_VERSION([0.19.8])
GETTEXT_PACKAGE=mlrpt
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package.])
AM_GNU_GETTEXT
PROGRAMNAME_LOCALEDIR=[${datadir}/locale]
AC_SUBST(PROGRAMNAME_LOCALEDIR)

# Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GNU_GETTEXT

# Checks for header files.
AC_CHECK_HEADERS([libintl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset sqrt strstr])
AC_CHECK_FUNCS([malloc])
AC_CHECK_FUNCS([realloc])

AC_CONFIG_FILES([Makefile
                 src/Makefile
				 po/Makefile.in])
AC_OUTPUT
