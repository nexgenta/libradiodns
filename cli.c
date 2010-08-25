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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>

#include "libradiodns.h"

static void
usage(const char *progname)
{
	fprintf(stderr, "Usage: %s domain DOMAIN\n", progname);
	fprintf(stderr, "Usage: %s fm FREQ PI COUNTRY [SUFFIX]\n", progname);
	fprintf(stderr, " - FREQ is specified in units of 10KHz\n"
			" - PI can be specified in decimal or hex (prefix with '0x')\n"
			" - COUNTRY must be an ISO country code or an RDS ECC (decimal or hex)\n");
	fprintf(stderr, "Usage: %s dvb ONID TSID SID NID [SUFFIX]\n", progname);
	fprintf(stderr, " - Each of ONID, TSID, SID and NID may be specified in either decimal or hex\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	radiodns_t *context;
	long freq, pi, onid, nid, sid, tsid, cval;
	const char *country, *suffix;
	char cbuf[16];
	char *endptr;
	
	if(argc < 2)
	{
		usage(argv[0]);
	}
	context = NULL;
	suffix = NULL;
	if(!strncmp(argv[1], "domain", strlen(argv[1])))
	{
		if(argc != 3)
		{
			usage(argv[0]);
		}
		context = radiodns_create(argv[1]);
	}
	else if(!strncmp(argv[1], "fm", strlen(argv[1])) || !strncmp(argv[1], "vhf", strlen(argv[1])))
	{
		if(argc < 5 || argc > 6)
		{
			usage(argv[0]);
		}
		endptr = NULL;
		freq = strtol(argv[2], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing FREQ at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		pi = strtol(argv[3], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing PI at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		if(isdigit(argv[4][0]) || (argv[4][0] == '0' && argv[4][0] == 'x'))
		{
			cval = strtol(argv[4], &endptr, 0);
			if(endptr && endptr[0])
			{
				fprintf(stderr, "%s: error parsing COUNTRY at '%s'\n", argv[0], endptr);
				usage(argv[0]);
			}
			printf("cval = %03x\n", cval);
			sprintf(cbuf, "%03x", (int) cval);
			country = cbuf;
		}
		else
		{
			country = argv[4];
		}
		if(argc == 5)
		{
			suffix = argv[5];
		}
		context = radiodns_create_fm(freq, pi, country, suffix);
	}
	else if(!strncmp(argv[1], "dvb", strlen(argv[1])))
	{
		if(argc < 6 || argc > 7)
		{
			usage(argv[0]);
		}
		onid = strtol(argv[2], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing ONID at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		tsid = strtol(argv[3], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing TSID at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		sid = strtol(argv[4], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing SID at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		nid = strtol(argv[5], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing NID at '%s'\n", argv[0], endptr);
			usage(argv[0]);
		}
		if(argc == 7)
		{
			suffix = argv[6];
		}
		context = radiodns_create_dvb(onid, tsid, sid, nid, suffix);
	}
	else
	{
		usage(argv[0]);
	}
	if(!context)
	{
		fprintf(stderr, "%s: failed to create context: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("Domain: %s\n", radiodns_domain(context));
	if(0 > radiodns_resolve_target(context))
	{
		fprintf(stderr, "%s: error resolving target: errno = %d, h_errno = %d\n", argv[0], errno, h_errno);
		radiodns_destroy(context);
		exit(EXIT_FAILURE);
	}
	printf("Target: %s\n", radiodns_target(context));
	radiodns_destroy(context);
	return 0;
}


