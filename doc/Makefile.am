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

man_MANS = radiodns_create.3 radiodns_destroy.3 radiodns_domain.3 \
	radiodns_target.3 radiodns_resolve_target.3 radiodns_resolve_app.3 \
	radiodns_destroy_app.3

## Distribute the manpages along with the source to save people needing
## docbook2x

EXTRA_DIST = $(man_MANS) \
	radiodns_create.xml radiodns_destroy.xml radiodns_domain.xml \
	radiodns_target.xml radiodns_resolve_target.xml radiodns_resolve_app.xml \
	radiodns_destroy_app.xml

if HAVE_DB2X

.mxml.3:
	$(db2x_manxml) $<

.xml.mxml:
	$(db2x_xsltproc) -s man $< -o $@

endif
