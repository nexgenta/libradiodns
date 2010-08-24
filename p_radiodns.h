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

#ifndef P_RADIODNS_H_
# define P_RADIODNS_H_                  1

# include <stdio.h>

# include <stdlib.h>
# include <string.h>
# include <ctype.h>
# include <netinet/in.h>
# include <arpa/nameser.h>
# include <resolv.h>
# include <netdb.h>
# include <errno.h>

# include "libradiodns.h"

# ifndef NETDB_INTERNAL
#  define NETDB_INTERNAL                -1
# endif

/* The maximum length of the components to the left of the "radiodns.org"
 * suffix in domain names.
 */
# define RADIODNS_MAX_CLEN              32

/* The default suffix */
# define RADIODNS_SUFFIX                "radiodns.org"
/* The default suffix for DVB */
# define RADIODNS_DVB_SUFFIX            "tvdns.net"

struct radiodns_struct
{
  char *domain;
  char *target;
  unsigned char *answer;
};

#endif /*!P_RADIODNS_H_*/
