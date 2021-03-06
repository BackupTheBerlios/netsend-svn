/*
** netsend - a high performance filetransfer and diagnostic tool
** http://netsend.berlios.de
**
**
** Copyright (C) 2006 - Hagen Paul Pfeifer <hagen@jauu.net>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/utsname.h>

#include <limits.h>

#ifndef ULLONG_MAX
# define ULLONG_MAX 18446744073709551615ULL
#endif

#include "global.h"
#include "xfuncs.h"


/* Simple malloc wrapper - prevent error checking */
void *
xmalloc(size_t size)
{

	void *ptr = malloc(size);

	if (!ptr)
		err_msg_die(EXIT_FAILMEM, "Out of mem: %s!\n", strerror(errno));
	return ptr;
}


void xgetaddrinfo(const char *node, const char *service,
		struct addrinfo *hints, struct addrinfo **res)
{
	int ret, ai_protocol, ai_socktype;
	struct addrinfo *a;
	bool dccp_sctp_fixup = false;

	if (hints) {
	/* getaddrinfo() does not support DCCP/SCTP yet, so fix things up manually 8-/ */
		switch (hints->ai_protocol) {
		case IPPROTO_DCCP:
		case IPPROTO_UDPLITE:
		case IPPROTO_SCTP:
			ai_protocol = hints->ai_protocol;
			ai_socktype = hints->ai_socktype;
			hints->ai_protocol = IPPROTO_TCP;
			hints->ai_socktype = hints->ai_protocol == IPPROTO_DCCP ?
							SOCK_DGRAM : SOCK_STREAM;

			dccp_sctp_fixup = true;
		}
	}

	ret = getaddrinfo(node, service, hints, res);
	if (ret != 0) {
		err_msg_die(EXIT_FAILNET, "Call to getaddrinfo() failed: %s!\n",
				(ret == EAI_SYSTEM) ?  strerror(errno) : gai_strerror(ret));
	}

	/* check if we meddled with *hints behind getaddrinfos back and fix things up again */
	if (!dccp_sctp_fixup)
		return; /* no we did not */

	/* yes, need fixup */
	for (a = *res; a != NULL ; a = a->ai_next) {
		a->ai_protocol = ai_protocol;
		a->ai_socktype = ai_socktype;
	}
}


void xsetsockopt(int s, int level, int optname, const void *optval, socklen_t optlen, const char *str)
{
	int ret = setsockopt(s, level, optname, optval, optlen);
	if (ret)
		err_sys_die(EXIT_FAILNET, "Can't set socketoption %s", str);
}


int xsnprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int len;

	va_start(ap, format);
	len = vsnprintf(str, size, format, ap);
	va_end(ap);
        if (len < 0 || ((size_t)len) >= size)
		err_msg_die(EXIT_FAILINT, "buflen %u not sufficient (ret %d)",
								size, len);
	return len;
}


char *xstrdup(const char *src)
{
	size_t len;
	char *duplicate;

	if (src == NULL)
		return NULL;

	len = strlen(src) + 1;

	if (!len) /* integer overflow */
		err_msg_die(EXIT_FAILINT, "xstrdup: string execeeds size_t range");
	duplicate = xmalloc(len);
	memcpy(duplicate, src, len);
	return duplicate;
}

void xfstat(int filedes, struct stat *buf, const char *s)
{
	if (fstat(filedes, buf))
		err_sys_die(EXIT_FAILMISC, "Can't fstat file %s", s);
}


void xpipe(int filedes[2])
{
	if (pipe(filedes))
		err_sys_die(EXIT_FAILMISC, "Can't create pipe");
}

/* vim:set ts=4 sw=4 tw=78 noet: */
