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

static const char *
check_suffix(const char *suffix)
{
  if(!suffix)
    {
      suffix = RADIODNS_SUFFIX;
    }
  while(suffix[0] == '.') suffix++;
  if(strlen(suffix) > MAXDNAME - RADIODNS_MAX_CLEN)
    {
      errno = EINVAL;
      return NULL;
    }
  return suffix;
}

/* Create a new RadioDNS context using a specified domain name */
radiodns_t *
radiodns_create(const char *domain)
{
  radiodns_t *context;

  if(NULL == (context = calloc(1, sizeof(radiodns_t))))
    {
      return NULL;
    }
  if(NULL == (context->domain = strdup(domain)))
    {
      free(context);
      return NULL;
    }
  return context;
}

/* Create a new RadioDNS context for a VHF/FM service
 * - country must be either a 2-letter country code or a 3-character
 *   RDS ECC.
 * - suffix may be specified to override the default 'radiodns.org',
 *   otherwise NULL
 */
radiodns_t *
radiodns_create_fm(unsigned int freq, unsigned int pi, const char *country, const char *suffix)
{
  char dname[MAXDNAME + 1];
 
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(strlen(country) == 3)
    {
      if(!isxdigit(country[0]) || !isxdigit(country[1]) || !isxdigit(country[2]))
	{
	  errno = EINVAL;
	  return NULL;
	}
    }
  else if(strlen(country) != 2)
    {
      errno = EINVAL;
      return NULL;
    }
  if(freq > 99999 || pi > 0xFFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  sprintf(dname, "%05d.%04x.%s.fm.%s", freq, pi, country, suffix);
  return radiodns_create(dname);
}

  /* Create a new RadioDNS context for a DAB service delivered via X-PAD */
radiodns_t *
radiodns_create_dab_xpad(unsigned int appty, unsigned int uatype, unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix)
{
  char dname[MAXDNAME + 1], scbuf[4], sbuf[10];
 
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(appty > 0xFF || uatype > 0xFFF || scids > 0xFFF || sid > 0xFFFFFFUL || eid > 0xFFFF || ecc > 0xFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  if(sid <= 0xFFFF)
    {
      sprintf(sbuf, "%04x", sid);
    }
  else
    {
      sprintf(sbuf, "%08x", sid);
    }
  if(scids <= 0xF)
    {
      sprintf(scbuf, "%x", scids);
    }
  else
    {
      sprintf(scbuf, "%03x", scids);
    }
  sprintf(dname, "%02x-%03x.%s.%s.%04x.%03x.dab.%s", appty, uatype, scbuf, sbuf, eid, ecc, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for a DAB service delivered via a SC */
radiodns_t *
radiodns_create_dab_sc(unsigned int pa, unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix)
{
  char dname[MAXDNAME + 1], scbuf[4], sbuf[10];
 
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(pa > 1023 || scids > 0xFFF || sid > 0xFFFFFFUL || eid > 0xFFFF || ecc > 0xFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  if(sid <= 0xFFFF)
    {
      sprintf(sbuf, "%04x", sid);
    }
  else
    {
      sprintf(sbuf, "%08x", sid);
    }
  if(scids <= 0xF)
    {
      sprintf(scbuf, "%x", scids);
    }
  else
    {
      sprintf(scbuf, "%03x", scids);
    }
  sprintf(dname, "%d.%s.%s.%04x.%03x.dab.%s", pa, scbuf, sbuf, eid, ecc, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for a DAB service delivered by neither
 * X-PAD nor an independent Service Component
 */
radiodns_t *
radiodns_create_dab(unsigned int scids, unsigned long sid, unsigned int eid, unsigned int ecc, const char *suffix)
{
  char dname[MAXDNAME + 1], scbuf[4], sbuf[10];
 
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(scids > 0xFFF || sid > 0xFFFFFFUL || eid > 0xFFFF || ecc > 0xFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  if(sid <= 0xFFFF)
    {
      sprintf(sbuf, "%04x", sid);
    }
  else
    {
      sprintf(sbuf, "%08x", sid);
    }
  if(scids <= 0xF)
    {
      sprintf(scbuf, "%x", scids);
    }
  else
    {
      sprintf(scbuf, "%03x", scids);
    }
  sprintf(dname, "%s.%s.%04x.%03x.dab.%s", scbuf, sbuf, eid, ecc, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for a DRM service */
radiodns_t *
radiodns_create_drm(unsigned long sid, const char *suffix)
{
  char dname[MAXDNAME + 1];
  
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(sid > 0xFFFFFFL)
    {
      errno = EINVAL;
      return NULL;
    }
  sprintf(dname, "%06x.drm.%s", sid, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for an AMSS service */
radiodns_t *
radiodns_create_amss(unsigned long sid, const char *suffix)
{
  char dname[MAXDNAME + 1];
  
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(sid > 0xFFFFFFL)
    {
      errno = EINVAL;
      return NULL;
    }
  sprintf(dname, "%06x.amss.%s", sid, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for an iBiquity HD Radio service */
radiodns_t *
radiodns_create_hdradio(unsigned long tx, unsigned int cc, const char *suffix)
{
  char dname[MAXDNAME + 1];
  
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(tx > 0xFFFFFL || cc > 0xFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  sprintf(dname, "%05x.%03x.hd.%s", tx, cc, suffix);
  return radiodns_create(dname);
}

/* Create a new RadioDNS context for a DVB service */
radiodns_t *
radiodns_create_dvb(unsigned int onid, unsigned int tsid, unsigned long sid, unsigned long nid, const char *suffix)
{
  char dname[MAXDNAME + 1];
 
  if(!suffix)
    {
      /* Until DVB/TVDNS/etc. is stabalised */
      suffix = RADIODNS_DVB_SUFFIX;
    }
  if(NULL == (suffix = check_suffix(suffix)))
    {
      return NULL;
    }
  if(onid > 0xFFFF || tsid > 0xFFFF || sid > 0xFFFF || nid > 0xFFFF)
    {
      errno = EINVAL;
      return NULL;
    }
  sprintf(dname, "%04x.%04x.%04x.%04x.dvb.%s", nid, sid, tsid, onid, suffix);
  return radiodns_create(dname);
}

/* Destroy an existing RadioDNS context */
void
radiodns_destroy(radiodns_t *context)
{
  if(context)
    {
      free(context->domain);
      free(context->target);
      free(context->answer);
      free(context);
    }
}

/* Return the domain name associated with the context */
const char *
radiodns_domain(radiodns_t *context)
{
  return context->domain;
}

/* Return the target domain name associated with the context */
const char *
radiodns_target(radiodns_t *context)
{
  return context->target;
}
