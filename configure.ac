dnl libradiodns: The RadioDNS helper library
dnl
dnl Copyright 2010 Mo McRoberts.
dnl
dnl  Licensed under the Apache License, Version 2.0 (the "License");
dnl  you may not use this file except in compliance with the License.
dnl  You may obtain a copy of the License at
dnl
dnl      http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl  Unless required by applicable law or agreed to in writing, software
dnl  distributed under the License is distributed on an "AS IS" BASIS,
dnl  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl  See the License for the specific language governing permissions and
dnl  limitations under the License.

AC_INIT([libradiodns],[master])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
BUILDTOOLS_INIT

AC_PROG_CC
AC_PROG_LIBTOOL

pkgconfigdir='$(libdir)/pkgconfig'
AC_SUBST([pkgconfigdir])

have_libresolv=no
glibc_resolver_hack=no
orig_LIBS="$LIBS"

AC_CHECK_LIB([resolv],[res_query],[have_libresolv=yes])
AC_CHECK_LIB([resolv],[__res_query],[have_libresolv=yes])

if test x"$have_libresolv" = x"yes" && test x"$host_os" = x"linux-gnu" ; then
   AC_CHECK_LIB([resolv],[ns_parserr],,[
     AC_CHECK_LIB([resolv],[__ns_parserr],,[glibc_resolver_hack=yes])
   ])
fi

if test x"$have_libresolv" = x"no" ; then
   AC_MSG_ERROR([cannot locate libresolv required to build libradiodns])
fi

if test x"$glibc_resolver_hack" = x"yes" ; then
   AC_MSG_RESULT([warning: old broken glibc -lresolv detected, forcing use of /usr/lib/libresolv.la])
   EXTRA_LIBS="/usr/lib/libresolv.a"
else
   RESOLVER_LIBS="-lresolv"
fi

AC_SUBST([RESOLVER_LIBS])
AC_SUBST([EXTRA_LIBS])
LIBS="$orig_LIBS"

have_db2x=no
AC_CHECK_PROG(db2x_xsltproc,db2x_xsltproc,db2x_xsltproc)
AC_CHECK_PROG(db2x_manxml,db2x_manxml,db2x_manxml)

if test x"$db2x_xsltproc" = x"" || test x"$db2x_manxml" = x"" ; then
   AC_MSG_RESULT([warning: docbook2x was not found; documentation will not be rebuilt])
else
   have_db2x=yes
fi
AM_CONDITIONAL([HAVE_DB2X],[test x"$have_db2x" = x"yes"])
AC_SUBST([db2x_xsltproc])
AC_SUBST([db2x_manxml])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile
libradiodns.pc
libradiodns-uninstalled.pc
doc/Makefile
buildtools/Makefile
buildtools/ext/Makefile
])

AC_OUTPUT
