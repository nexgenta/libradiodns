/*
 * libradiodns: The RadioDNS helper library
 *
 * Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef LIBRADIODNS_H_
# define LIBRADIODNS_H_                 1

typedef struct radiodns_struct radiodns_t;
typedef struct radiodns_app_struct radiodns_app_t;
typedef struct radiodns_srv_struct radiodns_srv_t;

struct radiodns_app_struct
{
	radiodns_app_t *next;
	char *name;
	char *txt;
	int nsrv;
	radiodns_srv_t *srv;
};

struct radiodns_srv_struct
{
	int priority;
	int weight;
	int port;
	char *target;
};

# ifdef __cplusplus
extern "C" {
# endif

	/* Create a new RadioDNS context using a specified domain name */
	radiodns_t *radiodns_create(const char *domain);
	
	/* Create a new RadioDNS context for a VHF/FM service
	 * - country must be either a 2-letter country code or a 3-character
	 *   RDS ECC.
	 * - suffix may be specified to override the default 'radiodns.org',
	 *   otherwise NULL
	 */
	radiodns_t *radiodns_create_fm(unsigned int freq, unsigned int pi, const char *country, const char *suffix);
	
	/* Create a new RadioDNS context for a DAB service delivered via X-PAD */
	radiodns_t *radiodns_create_dab_xpad(unsigned int appty, unsigned int uatype, unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix);
	
	/* Create a new RadioDNS context for a DAB service delivered via a SC */
	radiodns_t *radiodns_create_dab_sc(unsigned int pa, unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix);
	
	/* Create a new RadioDNS context for a DAB service delivered by neither
	 * X-PAD nor an independent Service Component
	 */
	radiodns_t *radiodns_create_dab(unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix);
	
	/* Create a new RadioDNS context for a DRM service */
	radiodns_t *radiodns_create_drm(unsigned long sid, const char *suffix);
	
	/* Create a new RadioDNS context for an AMSS service */
	radiodns_t *radiodns_create_amss(unsigned long sid, const char *suffix);
	
	/* Create a new RadioDNS context for an iBiquity HD Radio service */
	radiodns_t *radiodns_create_hdradio(unsigned long tx, unsigned int cc, const char *suffix);
	
	/* Create a new RadioDNS context for a DVB service */
	radiodns_t *radiodns_create_dvb(unsigned int onid, unsigned int tsid, unsigned long sid, unsigned long nid, const char *suffix);
	
	/* Destroy a RadioDNS context */
	void radiodns_destroy(radiodns_t *context);
	
	/* Determine the source domain name for a context, as set by one of the
	 * radiodns_create_xxx() functions
	 */
	const char *radiodns_domain(radiodns_t *context);
	
	/* Determine the target FQDN (beneath which SRV records are published)
	 * for a context.
	 */
	const char *radiodns_target(radiodns_t *context);
	
	/* Attempt to resolve the target FQDN for a context */
	const char *radiodns_resolve_target(radiodns_t *context);
	
	/* Locate all instances of the specified application for a context */
	radiodns_app_t *radiodns_resolve_app(radiodns_t *context, const char *name, const char *protocol);
	
	/* Destroy an application (list of instances) returned by
	 * radiodns_resolve_app()
	 */
	void radiodns_destroy_app(radiodns_app_t *app);

# ifdef __cplusplus
}
# endif

#endif /*!LIBRADIODNS_H_*/
