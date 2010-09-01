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

static int cmd_dns(int argc, char **argv);
static int cmd_fm(int argc, char **argv);
static int cmd_dab(int argc, char **argv);
static int cmd_drm(int argc, char **argv);
static int cmd_amss(int argc, char **argv);
static int cmd_hdradio(int argc, char **argv);
static int cmd_dvb(int argc, char **argv);
static int cmd_domain(int argc, char **argv);
static int cmd_target(int argc, char **argv);
static int cmd_verbose(int argc, char **argv);
static int cmd_quiet(int argc, char **argv);
static int cmd_app(int argc, char **argv);

struct command {
	const char *name;
	int (*fn)(int argc, char **argv);
	int nargs;
	int bare:1;
	int pre:1;
	int visible:1;
	const char *usage;
};

static struct command commands[] = {
	{ "dns", cmd_dns, 0, 1, 0, 1, "DOMAIN"},
	{ "fm", cmd_fm, 0, 1, 0, 1, "FREQ PI COUNTRY [SUFFIX]\n - FREQ is specified in units of 10KHz\n - COUNTRY must be an ISO country code or an RDS ECC\n"},
	{ "vhf", cmd_fm, 0, 1, 0, 0, "FREQ PI COUNTRY [SUFFIX]\n - FREQ is specified in units of 10KHz\n - COUNTRY must be an ISO country code or an RDS ECC\n"},
	{ "dab", cmd_dab, 0, 1, 0, 1, "SCIDS SID EID ECC [APPTYPE-UATYPE|PA] [SUFFIX]\n - APPTYPE-UATYPE must be specified in hexadecimal, with no prefix (e.g.,\n   1F-3C5)" },
	{ "drm", cmd_drm, 0, 1, 0, 1, "SID [SUFFIX]" },
	{ "dvb", cmd_dvb, 0, 1, 0, 1, "ONID TSID SID NID [SUFFIX]" },
	{ "amss", cmd_amss, 0, 1, 0, 1, "SID [SUFFIX]" },
	{ "hdradio", cmd_hdradio, 0, 1, 0, 1, "TX CC [SUFFIX]" },
	{ "target", cmd_target, 0, 0, 0, 1, NULL },
	{ "domain", cmd_domain, 0, 0, 0, 1, NULL },
	{ "verbose", cmd_verbose, 0, 0, 1, 1, NULL },
	{ "quiet", cmd_quiet, 0, 0, 1, 1, NULL },
	{ "app", cmd_app, 1, 0, 0, 1, "TYPE" },
	{ NULL, NULL, 0, 0, 0, 0, NULL },
};

const char *progname;
static struct command *current_command;
static radiodns_t *context;
static int verbose = -1;

static void
usage()
{
	if(current_command)
	{
		if(current_command->usage)
		{
			fprintf(stderr, "Usage: %s %s\n", current_command->name, current_command->usage);
		}
		else
		{
			fprintf(stderr, "No usage information available for '%s'\n", current_command->name);
		}
	}
	else
	{
		fprintf(stderr, "Usage: %s [OPTIONS] KIND ARGS...\n", progname);
		fprintf(stderr, " KIND is one of 'dns', 'fm', 'dab', 'drm', amss', or 'hdradio'\n");
		fprintf(stderr, " Unless otherwise stated, numeric values can be specified in decimal,\n"
				" hexadecimal (with a '0x' prefix), or octal (with a '0' prefix).\n\n");

		fprintf(stderr, "OPTIONS can include any of the following flags:\n");
		fprintf(stderr, " -quiet      Be quiet\n");
		fprintf(stderr, " -verbose    Be verbose\n\n");

		fprintf(stderr, "OPTIONS can also include any of the following commands:\n");
		fprintf(stderr, " -target     Print the application-discovery target domain name.\n");
		fprintf(stderr, " -domain     Print the source (constructed) domain name.\n");
		fprintf(stderr, " -app NAME   Look for instances of _NAME._tcp within the target domain.\n\n");

		fprintf(stderr, "If no OPTIONS are supplied, behaviour is as if -domain -target were\n");
		fprintf(stderr, "specified on the command-line, and the 'verbose' flag is enabled unless\n");
		fprintf(stderr, "-quiet is explicitly specified.\n");
	}
}

static int
check_context(radiodns_t *context)
{
	if(context)
	{
		return 0;
	}
	fprintf(stderr, "%s: failed to create context: %s\n", progname, strerror(errno));
	return 1;
}

static int
cmd_dns(int argc, char **argv)
{
	if(argc != 2)
	{
		usage();
		return 1;
	}
	if(context)
	{
		radiodns_destroy(context);
	}
	context = radiodns_create(argv[1]);
	return check_context(context);
}

static int
cmd_fm(int argc, char **argv)
{
	long freq, pi, cval;
	const char *country, *suffix;
	char *endptr, cbuf[16];

	suffix = NULL;
	if(argc < 4 || argc > 5)
	{
		usage();
		return 1;
	}
	endptr = NULL;
	freq = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing FREQ at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	pi = strtol(argv[2], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing PI at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(isdigit(argv[3][0]) || (argv[3][0] == '0' && argv[3][0] == 'x'))
	{
		cval = strtol(argv[3], &endptr, 0);
		if(endptr && endptr[0])
		{
			fprintf(stderr, "%s: error parsing COUNTRY at '%s'\n", argv[0], endptr);
			usage();
			return 1;
		}
		sprintf(cbuf, "%03x", (int) cval);
		country = cbuf;
	}
	else
	{
		country = argv[3];
	}
	if(argc == 5)
	{
		suffix = argv[4];
	}
	if(context)
	{
		radiodns_destroy(context);
	}
	context = radiodns_create_fm(freq, pi, country, suffix);
	return check_context(context);
}

static int
cmd_dab(int argc, char **argv)
{
	const char *suffix;
	char *endptr;
	long scids, sid, eid, ecc, apptype, uatype, pa;

	suffix = NULL;
	if(argc < 5 || argc > 7)
	{
		usage();
		return 1;
	}
	scids = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing SCIDS at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	sid = strtol(argv[2], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing SID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	eid = strtol(argv[3], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing EID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	ecc = strtol(argv[4], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing ECC at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(argc == 7)
	{
		suffix = argv[6];
	}
	if(argc >= 6)
	{
		if(strlen(argv[5]) <= 6 && strchr(argv[5], '-'))
		{
			/* APPTYPE-UATYPE */
			apptype = strtol(argv[5], &endptr, 16);
			if(!endptr || endptr[0] != '-')
			{
				fprintf(stderr, "%s: error parsing APPTYPE at '%s'\n", argv[0], endptr);
				usage(argv[0]);
			}
			endptr++;
			uatype = strtol(endptr, &endptr, 16);
			if(endptr && endptr[0])
			{
				fprintf(stderr, "%s: error parsing UATYPE at '%s'\n", argv[0], endptr);
				usage(argv[0]);
			}
			if(context)
			{
				radiodns_destroy(context);
			}
			context = radiodns_create_dab_xpad(apptype, uatype, scids, sid, eid, ecc, suffix);
		}
		else
		{
			pa = strtol(argv[5], &endptr, 16);
			if(endptr && endptr[0])
			{
				if(argc == 7)
				{
					fprintf(stderr, "%s: error parsing PA at '%s'\n", argv[0], endptr);
					usage();
					return 1;
				}
				suffix = argv[5];
				if(context)
				{
					radiodns_destroy(context);
				}
				context = radiodns_create_dab(scids, sid, eid, ecc, suffix);
			}
			else
			{
				if(context)
				{
					radiodns_destroy(context);
				}
				context = radiodns_create_dab_sc(pa, scids, sid, eid, ecc, suffix);
			}
		}
	}
	else
	{
		if(context)
		{
			radiodns_destroy(context);
		}
		context = radiodns_create_dab(scids, sid, eid, ecc, suffix);
	}			
	return check_context(context);
}

static int
cmd_drm(int argc, char **argv)
{
	long sid;
	char *endptr;
	const char *suffix;

	suffix = NULL;
	if(argc < 2 || argc > 3)
	{
		usage();
		return 1;
	}
	sid = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing SID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(argc == 4)
	{
		suffix = argv[3];
	}
	context = radiodns_create_drm(sid, suffix);
	return check_context(context);
}

static int
cmd_amss(int argc, char **argv)
{
	long sid;
	char *endptr;
	const char *suffix;

	suffix = NULL;
	if(argc < 2 || argc > 3)
	{
		usage();
		return 1;
	}
	sid = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing SID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(argc == 4)
	{
		suffix = argv[3];
	}
	context = radiodns_create_amss(sid, suffix);
	return check_context(context);
}

static int
cmd_hdradio(int argc, char **argv)
{
	long tx, cc;
	char *endptr;
	const char *suffix;

	suffix = NULL;
	if(argc < 3 || argc > 4)
	{
		usage();
		return 1;
	}
	tx = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing TX at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	cc = strtol(argv[2], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing CC at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(argc == 4)
	{
		suffix = argv[3];
	}
	context = radiodns_create_hdradio(tx, cc, suffix);
	return check_context(context);
}

static int
cmd_dvb(int argc, char **argv)
{
	long onid, tsid, sid, nid;
	const char *suffix;
	char *endptr;
	
	suffix = NULL;
	if(argc < 5 || argc > 6)
	{
		usage();
		return 1;
	}
	onid = strtol(argv[1], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing ONID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	tsid = strtol(argv[2], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing TSID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	sid = strtol(argv[3], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing SID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	nid = strtol(argv[4], &endptr, 0);
	if(endptr && endptr[0])
	{
		fprintf(stderr, "%s: error parsing NID at '%s'\n", argv[0], endptr);
		usage();
		return 1;
	}
	if(argc == 6)
	{
		suffix = argv[5];
	}
	context = radiodns_create_dvb(onid, tsid, sid, nid, suffix);
	return check_context(context);
}

static int
cmd_domain(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	if(verbose)
	{
		printf("Domain: ");
	}
	puts(radiodns_domain(context));
	return 0;
}

static int
cmd_target(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	if(0 > radiodns_resolve_target(context))
	{
		fprintf(stderr, "%s: error resolving target: errno = %d, h_errno = %d\n", progname, errno, h_errno);
		return 1;
	}
	if(verbose)
	{
		printf("Target: ");
	}
	puts(radiodns_target(context));
	return 0;
}

static int
cmd_verbose(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	verbose = 1;
	return 0;
}

static int
cmd_quiet(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	verbose = 0;
	return 0;
}

static int
cmd_app(int argc, char **argv)
{
	radiodns_app_t *app, *p;
	int c;

	if(argc != 2)
	{
		usage();
		return 1;
	}
	if(!(app = radiodns_resolve_app(context, argv[1], NULL)))
	{
		fprintf(stderr, "%s: no instances found\n", argv[1]);
		return 1;
	}
	for(p = app; p; p = p->next)
	{
		if(p->name)
		{
			printf("Instance \"%s\":\n", p->name);
		}
		else
		{
			printf("Instance (no name):\n");
		}
		if(p->nsrv == 1)
		{
			printf("  1 service record:\n");
		}
		else
		{
			printf("  %d service records:\n", p->nsrv);
		}
		for(c = 0; c < p->nsrv; c++)
		{
			printf("    IN SRV %d %d %d %s.\n", p->srv[c].priority, p->srv[c].weight, p->srv[c].port, p->srv[c].target);
		}
		if(!p->nparams)
		{
			printf("  No parameters.\n");
		}
		else if(p->nparams == 1)
		{
			printf("  1 parameter:\n");
		}
		else
		{
			printf("  %d parameters:\n", p->nparams);
		}
		for(c = 0; c < p->nparams; c++)
		{
			printf("    %s = %s\n", p->params[c].key, p->params[c].value);
		}
	}
	radiodns_destroy_app(app);
	return 0;
}

int
main(int argc, char **argv)
{
	int c, d, r, immediate;

	if(!(progname = argv[0]))
	{
		progname = "radiodns";
	}	
	immediate = 0;
	r = 0;
	for(c = 1; c < argc; c++)
	{
		if(argv[c][0] != '-')
		{
			break;
		}
		current_command = NULL;
		for(d = 0; commands[d].name; d++)
		{
			if(commands[d].bare)
			{
				continue;
			}
			if(!strncmp(commands[d].name, &(argv[c][1]), strlen(argv[c]) - 1))
			{
				if(current_command)
				{
					fprintf(stderr, "%s: '%s' is ambiguous\n", progname, argv[c]);
					exit(EXIT_FAILURE);
				}
				current_command = &(commands[d]);
			}
		}
		if(!current_command)
		{
			fprintf(stderr, "%s: no such option '%s'\n", progname, argv[c]);
			exit(EXIT_FAILURE);
		}
		if(argc - c - 1 < current_command->nargs)
		{
			if(current_command->usage)
			{
				fprintf(stderr, "Usage: %s -%s %s\n", progname, current_command->name, current_command->usage);
			}
			else if(current_command->nargs == 1)
			{
				fprintf(stderr, "%s: option '-%s' requires an argument\n", progname, current_command->name);
			}
			else
			{
				fprintf(stderr, "%s: option '-%s' requires %d arguments\n", progname, current_command->name, current_command->nargs);
			}
			exit(EXIT_FAILURE);
		}
		if(current_command->pre)
		{
			/* Execute it before doing anything else */
			if(current_command->fn(current_command->nargs + 1, &(argv[c])))
			{
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			/* Execute it immediately after parameter parsing */
			immediate = 1;
		}
		/* Skip any arguments */
		c += current_command->nargs;
	}
	current_command = NULL;
	if(c == argc)
	{
		/* In a future version, we might flip to an interactive
		 * mode, but not yet.
		 */
		usage();
		exit(EXIT_FAILURE);
	}
	for(d = 0; commands[d].name; d++)
	{
		if(!commands[d].bare)
		{
			continue;
		}
		if(!strncmp(commands[d].name, argv[c], strlen(argv[c])))
		{
			if(current_command)
			{
				fprintf(stderr, "%s: '%s' is ambiguous\n", progname, argv[c]);
				exit(EXIT_FAILURE);
			}
			current_command = &(commands[d]);
		}
	}
	if(!current_command)
	{
		fprintf(stderr, "%s: '%s' is not a known kind\n", progname, argv[c]);
		exit(EXIT_FAILURE);
	}
	if(current_command->fn(argc - c, &(argv[c])))
	{
		exit(EXIT_FAILURE);
	}
	if(immediate)
	{	
		if(verbose == -1)
		{
			verbose = 0;
		}
		/* Loop argc/argv a second time and execute the ones we skipped earlier */
		for(c = 1; c < argc; c++)
		{
			if(argv[c][0] != '-')
			{
				break;
			}
			current_command = NULL;
			for(d = 0; commands[d].name; d++)
			{
				if(commands[d].bare)
				{
					continue;
				}
				if(!strncmp(commands[d].name, &(argv[c][1]), strlen(argv[c]) - 1))
				{
					current_command = &(commands[d]);
				}
			}		
			if(!current_command->pre)
			{
				if(current_command->fn(current_command->nargs + 1, &(argv[c])))
				{
					exit(EXIT_FAILURE);
				}
			}
			/* Skip any arguments */
			c += current_command->nargs;
		}
	}
	else
	{
		/* Pretend that '-verbose -domain -target' was specified */
		if(verbose == -1)
		{
			verbose = 1;
		}
		cmd_domain(0, NULL);
		r = cmd_target(0, NULL);	   
	}
	radiodns_destroy(context);	
	return r;
}


