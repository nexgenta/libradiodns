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
#include <errno.h>
#include <netdb.h>

#include "libradiodns.h"

int
main(int argc, char **argv)
{
  int c;
  radiodns_t *context;

  if(argc < 2)
    {
      fprintf(stderr, "Usage: %s DOMAIN [DOMAIN ...]\n", argv[0]);
      return 1;
    }
  for(c = 1; c < argc; c++)
    {
      context = radiodns_create(argv[c]);
      printf("Domain: %s\n", radiodns_domain(context));
      if(0 > radiodns_resolve_target(context))
	{
	  fprintf(stderr, "Error resolving target; errno = %d, h_errno = %d\n", errno, h_errno);
	  radiodns_destroy(context);
	  continue;
	}
      printf("Target: %s\n", radiodns_target(context));
      radiodns_destroy(context);
    }
  return 0;
}


