## libradiodns: The RadioDNS helper library
##
## Copyright 2010 Mo McRoberts.
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

DIST_SUBDIRS = buildtools

EXTRA_DIST = LICENSE NOTICE README.GIT \
	libradiodns.pc.in libradiodns-uninstalled.pc.in

DISTCLEANFILES = libradiodns.pc libradiodns-uninstalled.pc

pkgconfig_DATA = libradiodns.pc

noinst_DATA = libradiodns-uninstalled.pc

include_HEADERS = libradiodns.h

lib_LTLIBRARIES = libradiodns.la

libradiodns_la_SOURCES = p_radiodns.h \
	context.c resolver.c

libradiodns_la_LDFLAGS = -avoid-version
libradiodns_la_LIBADD = @RESOLVER_LIBS@

bin_PROGRAMS = radiodns

radiodns_SOURCES = cli.c

radiodns_LDADD = libradiodns.la @EXTRA_LIBS@
radiodns_LDFLAGS = -static-libtool-libs


