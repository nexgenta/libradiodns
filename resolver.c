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
/* Maximum number of parameters supported */
#define RDNS_MAXPARAMS                  8

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
	r = 0; /* -1 == some catchable error, -2 == catastrophic error */
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
					r = -2;
					break;
				}
			}
			if(!(app = app_create()))
			{
				r = -2;
				break;
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
				radiodns_destroy_app(app);
				break;
			}	
		}
		else if(ns_rr_type(rr) == ns_t_txt)
		{
			if(!defapp)
			{
				if(!(defapp = app_create()))
				{
					r = -2;
					break;
				}
			}
			if(-2 == (r = app_parse_txt(defapp, handle, rr, dnbuf)))
			{
				break;
			}
		}
		else if(ns_rr_type(rr) == ns_t_srv)
		{
			if(!defapp)
			{
				if(!(defapp = app_create()))
				{
					r = -2;
					break;
				}
			}
			if(!defapp->srv)
			{
				if(!(defapp->srv = (radiodns_srv_t *) calloc(len, sizeof(radiodns_srv_t))))
				{
					r = -2;
					break;
				}
			}
			r = app_parse_srv(defapp, handle, rr, dnbuf, &(defapp->srv[defapp->nsrv]));
			if(r == 0)
			{
				defapp->nsrv++;
			}
			else if(r == -2)
			{
				break;
			}
		}
	}
	free(abuf);
	if(r == -2)
	{
		radiodns_destroy_app(defapp);
		radiodns_destroy_app(namedapps);
		return NULL;
	}
	if(defapp)
	{
		if(defapp->nsrv)
		{
			defapp->next = namedapps;
			return defapp;
		}
		radiodns_destroy_app(defapp);
	}
	/* In the event that there actually wasn't anything worth returning,
	 * don't confuse matters by leaving errno set to something random.
	 */
	errno = 0;
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
		free(app->_pbuf);
		free(app->params);
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
	size_t l;
	char *p;
	const char *t;
	radiodns_kv_t kv;
	int c;
	char hbuf[3];

	if(app->nparams == RDNS_MAXPARAMS)
	{
		/* Don't attempt to parse any more */
		return 0;
	}
	if(!app->params)
	{
		if(!(app->params = (radiodns_kv_t *) calloc(RDNS_MAXPARAMS, sizeof(radiodns_kv_t))))
		{
			return -2;
		}
	}
	l = app->_plen;
	if(!(p = (char *) realloc(app->_pbuf, l + strlen(txtrec) + 4)))
	{
		return -2;
	}
	if(app->_pbuf)
	{
		/* Adjust all of the pointers based on the realloc'd buffer */
		for(c = 0; c < app->nparams; c++)
		{
			app->params[c].key = p + (app->params[c].key - app->_pbuf);
			app->params[c].value = p + (app->params[c].value - app->_pbuf);
		}
	}
	app->_pbuf = p;
	p = &(p[l]);
	while(*txtrec && app->nparams < RDNS_MAXPARAMS)
	{
		while(isspace(*txtrec))
		{
			txtrec++;
		}
		if(!*txtrec)
		{
			break;
		}
		for(t = txtrec; *t && *t != '='; t++);
		if(!t)
		{
			break;
		}
		kv.key = p;
		t = txtrec;
		while(*t && *t != '=' && !isspace(*t))
		{
			if(*t == '%' && isxdigit(t[1]) && isxdigit(t[2]))
			{
				hbuf[0] = t[1];
				hbuf[1] = t[2];
				hbuf[2] = 0;
				c = strtol(hbuf, NULL, 16);
				*p = c;
				p++;
				t += 3;
				continue;
			}
			*p = *t;
			p++;
			t++;
		}
		*p = 0;
		p++;
		kv.value = p;
		while(*t != '=')
		{
			t++;
		}
		t++;
		while(*t && !isspace(*t))
		{
			if(*t == '%' && isxdigit(t[1]) && isxdigit(t[2]))
			{
				hbuf[0] = t[1];
				hbuf[1] = t[2];
				hbuf[2] = 0;
				c = strtol(hbuf, NULL, 16);
				*p = c;
				p++;
				t += 3;
				continue;
			}
			*p = *t;
			p++;
			t++;
		}
		*p = 0;
		p++;
		txtrec = t;
		app->params[app->nparams] = kv;
		app->nparams++;
	}
	*p = 0;
	p++;
	*p = 0;
	p++;
	app->_plen = p - app->_pbuf;
	return 0;
}

static int 
app_follow_ptr(radiodns_app_t *app, unsigned char *abuf, ns_msg phandle, ns_rr prr)
{
	char dnbuf[MAXDNAME + 1], dbuf[4];
	char *d, *p, *endp;
	ns_msg handle;
	ns_rr rr;
	int c, len, r;
	
	dn_expand(ns_msg_base(phandle), ns_msg_base(phandle) + ns_msg_size(phandle), ns_rr_rdata(prr), dnbuf, sizeof(dnbuf));	
	if(!(app->name = (char *) calloc(1, strlen(dnbuf))))
	{
		return -2;
	}
	d = app->name;
	p = dnbuf;
	while(*p && *p != '.')
	{
		if(*p == '\\' && isdigit(p[1]) && isdigit(p[2]) && isdigit(p[3]))
		{
			dbuf[0] = p[1];
			dbuf[1] = p[2];
			dbuf[2] = p[3];
			dbuf[3] = 0;
			c = strtol(dbuf, NULL, 10);
			*d = c;
			d++;
			p += 4;
			continue;
		}
		else if(*p == '\\' && p[1])
		{
			/* Skip the backslash, allowing it to escape the '.' */
			p++;
		}
		*d = *p;
		d++;
		p++;
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
