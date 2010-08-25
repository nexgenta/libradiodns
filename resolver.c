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

/* Attempt to resolve the target FQDN for a context */
const char *
radiodns_resolve_target(radiodns_t *context)
{
	nt len, c;
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
	if(!context->target)
	{
		if(NULL == radiodns_resolve_target(context))
		{
			return NULL;
		}
	}

	return NULL;
}

