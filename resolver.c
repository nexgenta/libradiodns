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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_radiodns.h"

/* Maximum answer buffer size -- 16 UDP packets should be sane */
#define RDNS_ANSWERBUFLEN               (512 * 16)

static radiodns_app_t *app_create(void);
static int app_parse_params(radiodns_app_t *app, const char *txtrec);
static int app_follow_ptr(radiodns_app_t *app, unsigned char *abuf, ns_msg handle, ns_rr rr);
static int app_parse_txt(radiodns_app_t *app, ns_msg handle, ns_rr rr, char *dnbuf);
static int app_parse_srv(radiodns_app_t *app, ns_msg handle, ns_rr rr, char *dnbuf, radiodns_srv_t *srv);

/* Attempt to resolve the target FQDN for a context */
const char *
radiodns_resolve_target(radiodns_t *context)
{
	int len, c;
	ns_msg handle;
	ns_rr rr;
	char domain[MAXDNAME + 1], dnbuf[MAXDNAME + 1];

	/* reset these to help with error handling in callers */
	h_errno = NETDB_INTERNAL;
	errno = 0;
	if(!context->answer)
	{
		if(NULL == (context->answer = (unsigned char *) malloc(RDNS_ANSWERBUFLEN)))
		{
			return NULL;
		}
	}
	free(context->target);
	context->target = NULL;
	strcpy(domain, context->domain);
	for(;;)
	{
		h_errno = 0;
		if(0 >= (len = res_query(domain, ns_c_in, ns_t_any, context->answer, RDNS_ANSWERBUFLEN)))
		{
			if(NETDB_INTERNAL == h_errno)
			{
				return NULL;
			}
			break;
		}
		if(0 > ns_initparse(context->answer, len, &handle))
		{
			break;
		}
		if(0 > (len = ns_msg_count(handle, ns_s_an)))
		{
			break;
		}
		/* Resolvers tend towards the sane: the last result in a set of
		 * DNAME and CNAME replies will be the one we care about. If
		 * you're building against a resolver which for some reason
		 * behaves differently, you'll need a workaround here.
		 */
		dnbuf[0] = 0;
		for(c = 0; c < len; c++)
		{
			if(ns_parserr(&handle, ns_s_an, c, &rr))
			{
				/* Parse failed? Hmm. */
				continue;
			}
			if(ns_rr_class(rr) != ns_c_in)
			{
				continue;
			}
			if(ns_rr_type(rr) == ns_t_dname || ns_rr_type(rr) == ns_t_cname)
			{
				dn_expand(ns_msg_base(handle), ns_msg_base(handle) + ns_msg_size(handle), ns_rr_rdata(rr), dnbuf, sizeof(dnbuf));
			}
		}      
		if(dnbuf[0])
		{
			if(0 == strcmp(domain, dnbuf))
			{
				break;
			}
			strcpy(domain, dnbuf);
			continue;
		}
		break;
	}
	/* Whatever we found last is the target, unless we found nothing at all,
	 * in which case the target is the same as the original domain.
	 */
	if(NULL == (context->target = strdup(domain)))
	{
		return NULL;
	}    
	return context->target;
}

/* Find all of the records for _<name>._<protocol>.<target> */
radiodns_app_t *
radiodns_resolve_app(radiodns_t *context, const char *name, const char *protocol)
{
	char fqdn[MAXDNAME+1], dnbuf[MAXDNAME+1];
	radiodns_app_t *defapp, *namedapps, *app;
	ns_msg handle;
	ns_rr rr;
	int len, c, r;
	unsigned char *abuf;

	if(!context->target)
	{
		if(NULL == radiodns_resolve_target(context))
		{
			return NULL;
		}
	}
	if(!protocol)
	{
		protocol = "tcp";
	}
	if(strlen(name) + strlen(protocol) + strlen(context->target) + 4 > MAXDNAME)
	{
		errno = ENAMETOOLONG;
		return NULL;
	}
	sprintf(fqdn, "_%s._%s.%s", name, protocol, context->target);
	if(0 >= (len = res_query(fqdn, ns_c_in, ns_t_any, context->answer, RDNS_ANSWERBUFLEN)))
	{
		return NULL;
	}	
	if(0 > ns_initparse(context->answer, len, &handle))
	{
		return NULL;
	}
	if(0 > (len = ns_msg_count(handle, ns_s_an)))
	{
		return NULL;
	}
	defapp = NULL;
	namedapps = NULL;
	abuf = NULL;
	for(c = 0; c < len; c++)
	{
		if(ns_parserr(&handle, ns_s_an, c, &rr))
		{
			/* Parse failed? Hmm. */
			continue;
		}
		if(ns_rr_class(rr) != ns_c_in)
		{
			continue;
		}
		if(ns_rr_type(rr) == ns_t_ptr)
		{
			if(!abuf)
			{
				if(!(abuf = (unsigned char *) malloc(RDNS_ANSWERBUFLEN)))
				{
					radiodns_destroy_app(namedapps);
					radiodns_destroy_app(defapp);
					return NULL;
				}
			}
			if(!(app = app_create()))
			{
				free(abuf);
				radiodns_destroy_app(namedapps);
				radiodns_destroy_app(defapp);
				return NULL;
			}
			r = app_follow_ptr(app, abuf, handle, rr);
			if(r == 0)
			{
				app->next = namedapps;
				namedapps = app;
			}
			else if(r == -1)
			{
				radiodns_destroy_app(app);
				continue;
			}
			else
			{
				free(abuf);
				radiodns_destroy_app(app);
				radiodns_destroy_app(namedapps);
				radiodns_destroy_app(defapp);
				return NULL;
			}	
		}
		else if(ns_rr_type(rr) == ns_t_txt)
		{
			if(!defapp)
			{
				if(!(defapp = app_create()))
				{
					radiodns_destroy_app(namedapps);
					free(abuf);
					return NULL;
				}
			}
			app_parse_txt(defapp, handle, rr, dnbuf);
		}
		else if(ns_rr_type(rr) == ns_t_srv)
		{
			if(!defapp)
			{
				if(!(defapp = app_create()))
				{
					radiodns_destroy_app(namedapps);
					free(abuf);
					return NULL;
				}
			}
			if(!defapp->srv)
			{
				if(!(defapp->srv = (radiodns_srv_t *) calloc(len, sizeof(radiodns_srv_t))))
				{
					radiodns_destroy_app(namedapps);
					radiodns_destroy_app(defapp);
					free(abuf);
					return NULL;
				}
			}
			r = app_parse_srv(defapp, handle, rr, dnbuf, &(defapp->srv[defapp->nsrv]));
			if(r == 0)
			{
				defapp->nsrv++;
			}
			else if(r == -2)
			{
				radiodns_destroy_app(namedapps);
				radiodns_destroy_app(defapp);
				free(abuf);
				return NULL;				
			}
		}
	}
	free(abuf);
	if(defapp)
	{
		defapp->next = namedapps;
		return defapp;
	}
	return namedapps;
}

void
radiodns_destroy_app(radiodns_app_t *app)
{
	radiodns_app_t *p;
	int c;

	while(app)
	{
		p = app->next;
		for(c = 0; c < app->nsrv; c++)
		{
			free(app->srv[c].target);
		}
		free(app->srv);
		free(app->name);
		free(app);
		app = p;
	}
}


static radiodns_app_t *
app_create(void)
{
	return calloc(1, sizeof(radiodns_app_t));
}

static int
app_parse_params(radiodns_app_t *app, const char *txtrec)
{
	printf("[TXT record = %s]\n", txtrec);
	return 0;
}

static int 
app_follow_ptr(radiodns_app_t *app, unsigned char *abuf, ns_msg phandle, ns_rr prr)
{
	char dnbuf[MAXDNAME + 1];
	char *d, *p;
	ns_msg handle;
	ns_rr rr;
	int c, len, r;

	dn_expand(ns_msg_base(phandle), ns_msg_base(phandle) + ns_msg_size(phandle), ns_rr_rdata(prr), dnbuf, sizeof(dnbuf));	
	if(!(app->name = (char *) calloc(1, strlen(dnbuf))))
	{
		return -2;
	}
	d = app->name;
	for(p = dnbuf; *p && *p != '.'; p++)
	{
		*d = *p;
		d++;
	}
	*d = 0;
	if(0 >= (len = res_query(dnbuf, ns_c_in, ns_t_any, abuf, RDNS_ANSWERBUFLEN)))
	{
		return -1;
	}
	if(0 > ns_initparse(abuf, len, &handle))
	{
		return -1;
	}
	if(0 > (len = ns_msg_count(handle, ns_s_an)))
	{
		return -1;
	}
	if(!(app->srv = (radiodns_srv_t *) calloc(len, sizeof(radiodns_srv_t))))
	{
		return -2;
	}
	for(c = 0; c < len; c++)
	{
		if(ns_parserr(&handle, ns_s_an, c, &rr))
		{
			continue;
		}
		if(ns_rr_class(rr) != ns_c_in)
		{
			continue;
		}
		if(ns_rr_type(rr) == ns_t_txt)
		{
			app_parse_txt(app, handle, rr, dnbuf);
		}
		if(ns_rr_type(rr) == ns_t_srv)
		{
			r = app_parse_srv(app, handle, rr, dnbuf, &(app->srv[app->nsrv]));
			if(r == 0)
			{
				app->nsrv++;
			}
			else
			{
				return r;
			}
		}
				
	}
	if(app->nsrv)
	{
		return 0;
	}
	return -1;
}

static int
app_parse_txt(radiodns_app_t *app, ns_msg handle, ns_rr rr, char *dnbuf)
{
	const unsigned char *p, *start;
	unsigned char l;
	int rrlen;

	(void) handle;

	p = start = ns_rr_rdata(rr);
	rrlen = ns_rr_rdlen(rr);
	while(p < start + rrlen)
	{
		l = *p;					
		p++;
		if(p + l > start + rrlen)
		{
			break;
		}
		memcpy(dnbuf, p, l);
		dnbuf[l] = 0;
		app_parse_params(app, dnbuf);
		p += l;
	}
	return 0;
}

static int
app_parse_srv(radiodns_app_t *app, ns_msg handle, ns_rr rr, char *dnbuf, radiodns_srv_t *srv)
{
	const unsigned char *rdata;
	
	(void) app;

	rdata = ns_rr_rdata(rr);
	srv->priority = ns_get16(rdata);
	rdata += NS_INT16SZ;

	srv->weight = ns_get16(rdata);
	rdata += NS_INT16SZ;
	
	srv->port = ns_get16(rdata);
	rdata += NS_INT16SZ;
	
	dn_expand(ns_msg_base(handle), ns_msg_base(handle) + ns_msg_size(handle), rdata, dnbuf, MAXDNAME);
	if(!(srv->target = strdup(dnbuf)))
	{
		return -2;
	}
	return 0;
}
