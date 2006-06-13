/*
** $Id$
**
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


#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
/* TODO: extend our configure script to loop for declaration */
#include <execinfo.h>

#include "global.h"

#define	MAXERRMSG 1024

static void
err_doit(int sys_error, const char *file, const int line_no,
		 const char *fmt, va_list ap)
{
	int	errno_save;
	char buf[MAXERRMSG];

	errno_save = errno;

#ifdef DEBUG
	snprintf(buf, sizeof(buf), "ERROR [%s:%d]: ", file, line_no);
#else
	snprintf(buf, sizeof(buf), "ERROR: ");
#endif

	vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ap);

	if (sys_error) {
		snprintf(buf + strlen(buf),  sizeof(buf) - strlen(buf), " (%s)",
				strerror(errno_save));
	}

	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(NULL);
}

void
x_err_ret(const char *file, int line_no, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, file, line_no, fmt, ap);
	va_end(ap);
	return;
}


void
x_err_sys(const char *file, int line_no, const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, file, line_no, fmt, ap);
	va_end(ap);
}

void print_bt(void)
{
	void *bt[128];
	int bt_size;
	char **bt_syms;
	int i;

	bt_size = backtrace(bt, 128);
	bt_syms = backtrace_symbols(bt, bt_size);
	fputs("BACKTRACE:\n", stderr);
	for(i = 1; i < bt_size; i++) {
		fprintf(stderr, "#%2d  %s\n", i - 1, bt_syms[i]);
	}
	fputs("\n", stderr);
}


/* vim:set ts=4 sw=4 tw=78 noet: */
