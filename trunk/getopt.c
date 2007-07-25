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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sched.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "global.h"
#include "xfuncs.h"


#ifdef HAVE_AF_TIPC
#include <linux/tipc.h>

static const struct {
	int  socktype;
	const char *sockname;
} socktype_map[] =
{
	{ SOCK_RDM, "SOCK_RDM" },
	{ SOCK_DGRAM, "SOCK_DGRAM" },
	{ SOCK_STREAM, "SOCK_STREAM" },
	{ SOCK_SEQPACKET, "SOCK_SEQPACKET" }
};


static void tipc_print_socktypes(void)
{
	unsigned i;
	for (i=0; i < ARRAY_SIZE(socktype_map); i++)
		fprintf(stderr, "%s\n", socktype_map[i].sockname);
}


static int tipc_parse_socktype(const char *socktype)
{
	unsigned i;

	for (i=0; i < ARRAY_SIZE(socktype_map); i++) {
		if (strcasecmp(socktype, socktype_map[i].sockname) == 0)
			return socktype_map[i].socktype;
	}
	fprintf(stderr, "Unknown socket type: %s. Known Types:\n", socktype);
	tipc_print_socktypes();
	exit(EXIT_FAILOPT);
}
#endif


#ifndef SCHED_BATCH
# define SCHED_BATCH 3
#endif

struct sched_policymap_t
{
	const int  no;
	const char *name;
} sched_policymap[] =
{
	{ SCHED_OTHER, "SCHED_OTHER" },
	{ SCHED_FIFO,  "SCHED_FIFO"  },
	{ SCHED_RR,    "SCHED_RR"    },
	{ SCHED_BATCH, "SCHED_BATCH" },
	{ 0, NULL },
};


extern struct opts opts;
extern struct conf_map_t congestion_map[];
extern struct conf_map_t memadvice_map[];
extern struct conf_map_t io_call_map[];
extern struct socket_options socket_options[];

void
usage(void)
{
	fprintf(stderr,
			"USAGE: %s [options] ( -t <input-file> <hostname>  |  -r <output-file> <multicast>\n\n"
			"    -t <input-file>  <hostname>     _t_ransmit input-file to host hostname\n"
			"    -r <output-file> <multicast>    _r_eceive data and save to outputfile\n"
			"                                    bind local socket to multicast group\n"
			"                                    (output-file and multicast adddr in receive mode are optional)\n\n"
			"-m <tcp | udp | dccp | sctp | udplite"
#ifdef HAVE_AF_TIPC
			" | tipc socktype"
#endif
			">    specify default transfer protocol (default: tcp)\n"
			"-p <port>                set portnumber (default: 6666)\n"
			"-u <sendfile | mmap | rw | read | splice >\n"
			"                         utilize specific function-call for IO operation\n"
			"                         (this depends on operation mode (-t, -r)\n"
			"-N                       select buffer chunk size (e.g. sendfile transfer amount per call)\n"
			"-n                       set maximum number of sized chunkes. This limit upper transfer limit\n"
			"-o <optname>             specify setsockopt(2) options\n"
			"-a <advice>              set memory advisory information\n"
			"          * normal\n"
			"          * sequential\n"
			"          * random\n"
			"          * willneed\n"
			"          * dontneed\n"
			"          * noreuse (equal to willneed for -u mmap)\n" /* 2.6.16 treats them the same */
			"-c <congestion>          set the congestion algorithm\n"
			"       available algorithms (kernelversion >= 2.6.16)\n"
			"          * bic\n"
			"          * cubic\n"
			"          * highspeed\n"
			"          * htcp\n"
			"          * hybla\n"
			"          * scalable\n"
			"          * vegas\n"
			"          * westwood\n"
			"          * reno\n"
			"-P <sched_policy> <priority>\n"
			"       sched_policy:\n"
			"          * SCHED_OTHER\n"
			"          * SCHED_FIFO\n"
			"          * SCHED_RR\n"
			"          * SCHED_BATCH (since kernel 2.6.16)\n"
			"       priority: MAX or MIN\n"
			"-C <int>                 change nice value of process\n"
			"-D                       no delay socket option (disable Nagle Algorithm)\n"
			"-N <int>                 specify how big write calls are (default: 8 * 1024)\n"
			"-e                       reuse port\n"
			"-6                       prefer ipv6\n"
			"-E <command>             execute command in server-mode and bind STDIN\n"
			"-R <string>		  set rtt data, e.g: -R 10n,10d,10m,10f\n"
			"			  where n: num iterations; d size; f: force RTT\n"
			"                         and STDOUT to program\n"
			"-T                       print statistics\n"
			"-8                       print statistic in bit instead of byte\n"
			"-I                       print statistic values in si prefixes (10**n, default: 2**n)\n"
			"-v                       make output verbose (vv even more verbose)\n"
			"-M                       print statistic in a machine parseable form\n"
			"-V                       print version\n"
			"-h                       print this help screen\n"
			"*****************************************************\n"
			"Bugs, hints or else, feel free and look at http://netsend.berlios.de\n"
			"SEE MAN-PAGE FOR FURTHER INFORMATION\n", opts.me);
}


static int
parse_rtt_string(const char *rtt_cmd)
{
	const char *tok = rtt_cmd;
	char *what;

	while (tok) {
		long value = strtol(tok, &what, 10);
		switch (*what) {
		case 'n':
			opts.rtt_probe_opt.iterations = value;
			if (value <= 0 || value > 100) {
				fprintf(stderr, "You want %ld rtt probe iterations - that's not sensible! "
						"Valid range is between 1 and 100 probe iterations\n", value);
				return -1;
			}
			break;
		case 'd':
			opts.rtt_probe_opt.data_size = value;
			if (value <= 0) {
				fprintf(stderr, "%ld not a valid data size for our rtt probe\n", value);
				return -1;
			}
			break;
		case 'm':
			opts.rtt_probe_opt.deviation_filter = value;
			if (value < 0 || value > 50) {
				fprintf(stderr, "%ldms are nonsensical for the filter multiplier (default is %d)\n", value, DEFAULT_RTT_FILTER);
				return -1;
			}
			break;
		case 'f':
			opts.rtt_probe_opt.force_ms = value;
			if (value < 0) {
				fprintf(stderr, "%ldms are nonsensical for a rtt\n", value);
				return -1;
			}
			break;
		default:
			fprintf(stderr, "short rtt option %s in %s not supported: %c not recognized\n", tok, rtt_cmd, *what);
			return -1;
		}
		if (*what) what++;
		if (*what && *what != ',') {
			fprintf(stderr, "rtt options must be comma seperated, got %c: %s\n", *what, tok);
			return -1;
		}
		tok = strchr(tok, ',');
		if (tok) tok++;
	}
	return 0;
}


static void parse_ipprotocol(const char *protoname)
{
	unsigned i;
	static const struct { const char *name; int socktype; int protocol; }
		ipprotocols[] = {
			{ "tcp", SOCK_STREAM, IPPROTO_TCP },
			{ "udp", SOCK_DGRAM, IPPROTO_UDP },
			{ "udplite", SOCK_DGRAM, IPPROTO_UDPLITE },
			{ "sctp", SOCK_STREAM, IPPROTO_SCTP },
			{ "dccp", SOCK_DCCP, IPPROTO_DCCP }
		};

	for (i=0; i < ARRAY_SIZE(ipprotocols); i++) {
		if (strcasecmp(ipprotocols[i].name, protoname))
			continue;
		opts.protocol = ipprotocols[i].protocol;
		opts.socktype = ipprotocols[i].socktype;
		return;
	}
	err_msg_die(EXIT_FAILOPT, "Protocol \"%s\" not supported", protoname);
}


/* parse_short_opt is a helper function who
** parse the particular options
*/
static int
parse_short_opt(char **opt_str, int *argc, char **argv[])
{
	int i;

	switch((*opt_str)[1]) {
		case 'r':
			opts.workmode = MODE_RECEIVE;
			break;
		case 't':
			opts.workmode = MODE_TRANSMIT;
			break;
		case '6':
			opts.family = AF_INET6;
			break;
		case '4':
			opts.family = AF_INET;
			break;
		case 'v':
			opts.verbose++;
			break;
		case 'T':
			opts.statistics++;
			break;
		case 'M':
			opts.machine_parseable++;
			break;
		case '8':
			opts.stat_unit = BIT_UNIT;
			break;
		case 'I':
			opts.stat_prefix = STAT_PREFIX_SI;
			break;
		case 'm':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
/* special case: TIPC is no IP protocol and we need to setup some additional things. */
#ifdef HAVE_AF_TIPC
			if (!strcasecmp((*argv)[2], "tipc")) {
				if (*argc <= 3) {
					fputs("tipc requires socket-type argument, known values:\n", stderr);
					tipc_print_socktypes();
					exit(EXIT_FAILOPT);
				}
				opts.family = AF_TIPC;
				opts.protocol = 0;
				opts.socktype = tipc_parse_socktype((*argv)[3]);
				(*argc) -=2;
				(*argv) += 2;
				return 0;
			}
#endif
		parse_ipprotocol((*argv)[2]);
		(*argc)--;
		(*argv)++;
		break;
		case 'a':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			for (i = 0; i <= MEMADV_MAX; i++ ) {
				if (!strcasecmp((*argv)[2], memadvice_map[i].conf_string)) {
					opts.mem_advice = memadvice_map[i].conf_code;
					opts.change_mem_advise++;
					(*argc)--;
					(*argv)++;
					return 0;
				}
			}
			err_msg_die(EXIT_FAILOPT, "ERROR: Mem Advice %s not supported!", (*argv)[2]);
			break;
		case 'c':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			for (i = 0; i <= CA_MAX; i++ ) {
				if (!strncasecmp((*argv)[2],
						congestion_map[i].conf_string,
						max(strlen(congestion_map[i].conf_string),
							strlen((*argv)[2]))))
				{
					opts.congestion = congestion_map[i].conf_code;
					opts.change_congestion++;
					(*argc)--;
					(*argv)++;
					return 0;
				}
			}
			err_msg_die(EXIT_FAILOPT, "ERROR: Congestion algorithm %s not supported!",
					(*argv)[2]);
			break;
		case 'R':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));

			if (parse_rtt_string((*argv)[2]) < 0)
				err_msg_die(EXIT_FAILOPT, "ERROR: Failure in rtt probe string %s !", (*argv[2]));

			(*argc)--;
			(*argv)++;
			return 0;
			break;
		case 'X':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));

			for (i = 0; i <= CA_MAX; i++ ) {
				if (!strncasecmp((*argv)[2],
							congestion_map[i].conf_string,
							max(strlen(congestion_map[i].conf_string),
								strlen((*argv)[2]))))
				{
					opts.congestion = congestion_map[i].conf_code;
					opts.change_congestion++;
					(*argc)--;
					(*argv)++;
					return 0;
				}
			}
			err_msg_die(EXIT_FAILOPT, "ERROR: Congestion algorithm %s not supported!",
										(*argv)[2]);
			break;
		case 'p':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			/* we allocate room for DEFAULT_PORT at initialize phase
			** now free it and reallocate a proper size
			*/
			free(opts.port);
			opts.port = xstrdup((*argv)[2]);
			(*argc)--;
			(*argv)++;
			break;
		case 'D':
			socket_options[CNT_TCP_NODELAY].value = 1;
			socket_options[CNT_TCP_NODELAY].user_issue = 1;
			break;
		case 'e':
			socket_options[CNT_SO_REUSEADDR].value = 1;
			socket_options[CNT_SO_REUSEADDR].user_issue = 1;
			break;
		case 'h':
			usage();
			exit(0);
			break;
		case 'V':
			printf("%s -- %s\n", PROGRAMNAME, VERSIONSTRING);
			exit(EXIT_OK);
			break;
		case 'E':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			opts.execstring = xstrdup((*argv)[2] + 1);
			strcpy(opts.outfile, (*argv)[2]);
			(*argc)--;
			(*argv)++;
			break;
		case 'n':
			if (((*opt_str)[2])  || ((*argc) <= 2)) {
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
				exit(EXIT_FAILOPT);
			}
			opts.multiple_barrier = strtol((*argv)[2], (char **)NULL, 10);
			(*argc)--;
			(*argv)++;
			break;
		case 'N':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));

			opts.buffer_size = strtol((*argv)[2], (char **)NULL, 10);
			if (opts.buffer_size <= 0)
				err_msg_die(EXIT_FAILOPT, "Buffer size to small (%d byte)!",
						opts.buffer_size);
			(*argc)--;
			(*argv)++;
			break;
		case 'u':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			for (i = 0; i <= IO_MAX; i++ ) {
				if (!strncasecmp((*argv)[2],
							io_call_map[i].conf_string,
							max(strlen(io_call_map[i].conf_string),
								strlen((*argv)[2]))))
				{
					opts.io_call = io_call_map[i].conf_code;
					(*argc)--;
					(*argv)++;
					return 0;
				}
			}
			err_msg_die(EXIT_FAILOPT, "ERROR: IO Function %s not supported!",
					(*argv)[2]);
			break;
			/* Two exception for this socketoption switch:
			 ** o Common used options like SO_REUSEADDR or SO_KEEPALIVE
			 **   can also be selected via a single short option
			 **   (like option "-e" for SO_REUSEADDR)
			 ** o Some socket options like TCP_INFO, TCP_MAXSEG or TCP_CORK
			 **   can't be useful utilized - we don't support these options
			 **   via this interface.
			 */
		case 'o':
			if (((*opt_str)[2])  || ((*argc) <= 3))
				err_msg_die(EXIT_FAILOPT, "Option error (%c:%d)",
						(*opt_str)[2], (*argc));
			/* parse socket option argument */
			for (i = 0; socket_options[i].sockopt_name; i++) {
				/* found option */
				if (!strcasecmp((*argv)[2], socket_options[i].sockopt_name)) {
					switch (socket_options[i].sockopt_type) {
						case SVT_BOOL:
						case SVT_ON:
							if (!strcasecmp((*argv)[3], "on")) {
								socket_options[i].value = 1;
							} else if (!strcasecmp((*argv)[3], "1")) {
								socket_options[i].value = 1;
							} else if (!strcasecmp((*argv)[3], "off")) {
								socket_options[i].value = 0;
							} else if (!strcasecmp((*argv)[3], "0")) {
								socket_options[i].value = 0;
							} else {
								fprintf(stderr, "ERROR: socketoption %s value %s "
										" not supported!\n", (*argv)[2], (*argv)[3]);
								exit(EXIT_FAILOPT);
							}
							socket_options[i].user_issue++;
							break;
						case SVT_INT:
							/* TODO: add some input checkings here */
							socket_options[i].value =
								strtol((*argv)[3], (char **)NULL, 10);
							socket_options[i].user_issue++;
							break;
						default:
							err_msg_die(EXIT_FAILMISC, "ERROR: Programmed Error (%s:%d)\n",
									__FILE__, __LINE__);
					}
					/* Fine, we are done ... */
					break;
				}
			}
			/* If we reach the end of our socket_options struct.
			** We found no matching socketoption because we didn't
			** support this particular option or the user smoke more
			** pot then the programmer - just kidding ... ;-)
			*/
			if (!socket_options[i].sockopt_name) {
				err_msg("socketoption \"%s\" not supported!", (*argv)[2]);
				fputs("Known Options:\n", stderr);
				for (i = 0; socket_options[i].sockopt_name; i++) {
					fputs(socket_options[i].sockopt_name, stderr);
					putc('\n', stderr);
				}
				exit(EXIT_FAILOPT);
			}
			(*argc) -= 2;
			(*argv) += 2;
			break;

		/* Scheduling policy menu[tm] */
		case 'P':
			if (((*opt_str)[2])  || ((*argc) <= 3)) {
				fprintf(stderr, "Option error (%c:%d)\n",
						(*opt_str)[2], (*argc));
				fprintf(stderr, "-P <POLICY> <PRIORITY>\n POLICY:\n");
				i = 0;
				while (sched_policymap[i++].name)
					fprintf(stderr, "  * %s\n", sched_policymap[i].name);
				fprintf(stderr, " PRIORITY:\n * MAX\n * MIN\n");
				exit(EXIT_FAILOPT);
			}
			/* parse socket option argument */
			for (i = 0; sched_policymap[i].name; i++) {
				if (!strcasecmp((*argv)[2], sched_policymap[i].name)) {
					opts.sched_policy = sched_policymap[i].no;
					opts.sched_user++;
				}
			}

			if (!opts.sched_user) {
				fprintf(stderr, "Not a valid scheduling policy (%s)!\nValid:\n",
						(*argv)[2]);
				i = 0;
				while (sched_policymap[i++].name)
					fprintf(stderr, " * %s\n", sched_policymap[i].name);
				exit(EXIT_FAILOPT);
			}

			/* user choosed a valid scheduling policy now the last
			** argument, the nicelevel must also match.
			** e.g. (-20 .. 19, priority MAX, MIN);
			*/
			if (!strcasecmp((*argv)[3], "MAX")) {
				opts.priority = sched_get_priority_max(opts.sched_policy);
			} else if (!strcasecmp((*argv)[3], "MIN")) {
				opts.priority = sched_get_priority_min(opts.sched_policy);
			} else {
				err_msg_die(EXIT_FAILOPT, "Not a valid scheduling priority (%s)!\n"
						"Valid: max or min", (*argv)[3]);
			}
			(*argc) -= 2;
			(*argv) += 2;
			break;

		case 'C':
			if (((*opt_str)[2])  || ((*argc) <= 2))
				err_msg_die(EXIT_FAILOPT, "option error (%c:%d)",
						(*opt_str)[2], (*argc));
			opts.nice = strtol((*argv)[2], (char **)NULL, 10);
			(*argc)--;
			(*argv)++;
			break;

		default:
			err_msg_die(EXIT_FAILOPT, "Short option %c not supported!\n", (*opt_str)[1]);
	}

	return 0;
}

/* Parse our command-line options and set some default options
** Honorable tests adduced that command-lines like e.g.:
** ./netsend -4 -6 -vvvo tcp_nodelay off -o SO_KEEPALIVE 1 \
**           -o SO_RCVBUF 65535  -u mmap -vp 444 -m tcp    \
**			 -c bic ./netsend.c
** are ready to run - I swear! ;-)
*/
int
parse_opts(int argc, char *argv[])
{
	int ret = 0;
	char *opt_str, *tmp;

	/* Zero out opts struct and set program name */
	memset(&opts, 0, sizeof(struct opts));
	if ((tmp = strrchr(argv[0], '/')) != NULL) {
		tmp++;
		opts.me = xstrdup(tmp);
	} else {
		opts.me = xstrdup(argv[0]);
	}

	/* Initialize some default values */
	opts.workmode    = MODE_NONE;
	opts.io_call     = IO_SENDFILE;
	opts.protocol    = IPPROTO_TCP;
	opts.socktype    = SOCK_STREAM;
	opts.family      = AF_UNSPEC;
	opts.stat_unit   = BYTE_UNIT;
	opts.stat_prefix = STAT_PREFIX_BINARY;
	opts.buffer_size = 0; /* we use default values (sendfile(): unlimited) */

	/* default behaviour is to probe the rtt: 10 times with a 500 payload packet */
	opts.rtt_probe_opt.iterations = 10;
	opts.rtt_probe_opt.data_size = 500;
	opts.rtt_probe_opt.force_ms = 0;
	/* our threshold filter */
	opts.rtt_probe_opt.deviation_filter = DEFAULT_RTT_FILTER;

	/* if opts.nice is equal INT_MAX nobody change something - hopefully */
	opts.nice = INT_MAX;
	opts.port = xstrdup(DEFAULT_PORT);

	/* outer loop runs over argv's ... */
	while (argc > 1) {

		opt_str = argv[1];

		if ((opt_str[0] == '-' && !opt_str[1]) ||
		     opt_str[0] != '-') {
			break;
		}

		/* ... inner loop - read command line switches and
		** correspondent arguments (e.g. -s SOCKETOPTION VALUE)
		*/
		while (isalnum(opt_str[1])) {
			parse_short_opt(&opt_str, &argc, &argv);
			opt_str++;
		}

		argc--;
		argv++;
	}

	/* OK - parsing the command-line seems fine!
	** Last but not least we drive some consistency checks
	*/
	if (opts.workmode == MODE_TRANSMIT) {

		if (argc <= 2) {
			err_msg("filename and hostname missing!");
			usage();
			exit(EXIT_FAILOPT);
		}

		switch (opts.io_call) { /* only sendfile(), mmap(), ... allowed */
			case IO_SENDFILE:
			case IO_SPLICE:
			case IO_MMAP:
			case IO_RW:
				break;
			default:
				opts.io_call = IO_SENDFILE;
				break;
		}

		opts.infile = xstrdup(argv[1]);
		opts.hostname = xstrdup(argv[2]);

	} else if (opts.workmode == MODE_RECEIVE) { /* MODE_RECEIVE */

		switch (--argc) {
			case 0: /* nothing to do */
				break;

			case 2:
				opts.hostname = xstrdup(argv[2]);
				/* fallthrough */
			case 1:
				opts.outfile = xstrdup(argv[1]);
				break;
			default:
				err_msg("You specify to many arguments!");
				usage();
				exit(EXIT_FAILINT);
				break;

		}

		switch (opts.io_call) { /* read() allowed */
			case IO_READ:
				break;
			default:
				opts.io_call = IO_READ;
		}
	} else {
		err_msg("You must specify your work mode: receive "
				"or transmit (-r | -t)!");
		usage();
		exit(EXIT_FAILOPT);
	}

	return ret;
}

/* vim:set ts=4 sw=4 tw=78 noet: */
