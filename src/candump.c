#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <socket-can/can.h>
#include <can_config.h>

extern int optind, opterr, optopt;

static int	s = -1;
static int	running = 1;

enum
{
	VERSION_OPTION = CHAR_MAX + 1,
	FILTER_OPTION,
};

static void print_usage(char *prg)
{
        fprintf(stderr, "Usage: %s <can-interface> [Options]\n"
                        "Options:\n"
	                " -f, --family=FAMILY\t"	"protocol family (default PF_CAN = %d)\n"
                        " -t, --type=TYPE\t"		"socket type, see man 2 socket (default SOCK_RAW = %d)\n"
                        " -p, --protocol=PROTO\t"	"CAN protocol (default CAN_PROTO_RAW = %d)\n"
			"     --filter=id:mask[:id:mask]...\n"
			"\t\t\t"			"apply filter\n"
			" -h, --help\t\t"		"this help\n"
			" -o <filename>\t\t"		"output into filename\n"
			" -d\t\t\t"			"daemonize\n"
			"     --version\t\t"		"print version information and exit\n",
		prg, PF_CAN, SOCK_RAW, CAN_PROTO_RAW);
}

static void sigterm(int signo)
{
	running = 0;
}

static struct can_filter *filter = NULL;
static int filter_count = 0;

int add_filter(u_int32_t id, u_int32_t mask)
{
	filter = realloc(filter, sizeof(struct can_filter) * (filter_count + 1));
	if(!filter)
		return -1;

	filter[filter_count].can_id = id;
	filter[filter_count].can_mask = mask;
	filter_count++;

	printf("id: 0x%08x mask: 0x%08x\n",id,mask);
	return 0;
}

#define BUF_SIZ	(255)

int main(int argc, char **argv)
{
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_PROTO_RAW;
	int opt, optdaemon = 0;
	struct sockaddr_can addr;
	struct can_frame frame;
	int nbytes, i;
	struct ifreq ifr;
	char *ptr;
	char *optout = NULL;
	u_int32_t id, mask;
	FILE *out = stdout;
	char buf[BUF_SIZ];
	int n = 0, err;

	signal(SIGPIPE, SIG_IGN);

	struct option		long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "family", required_argument, 0, 'f' },
		{ "protocol", required_argument, 0, 'p' },
		{ "type", required_argument, 0, 't' },
		{ "filter", required_argument, 0, FILTER_OPTION },
		{ "version", no_argument, 0, VERSION_OPTION},
		{ 0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "f:t:p:o:d", long_options, NULL)) != -1) {
		switch (opt) {
		case 'd':
			optdaemon++;
			break;

		case 'h':
			print_usage(basename(argv[0]));
			exit(0);

		case 'f':
			family = strtoul(optarg, NULL, 0);
			break;

		case 't':
			type = strtoul(optarg, NULL, 0);
			break;

		case 'p':
			proto = strtoul(optarg, NULL, 0);
			break;

		case 'o':
			optout = optarg;
			break;

		case FILTER_OPTION:
			ptr = optarg;
			while(1) {
				id = strtoul(ptr, NULL, 0);
				ptr = strchr(ptr, ':');
				if(!ptr) {
					fprintf(stderr, "filter must be applied in the form id:mask[:id:mask]...\n");
					exit(1);
				}
				ptr++;
				mask = strtoul(ptr, NULL, 0);
				ptr = strchr(ptr, ':');
				add_filter(id,mask);
				if(!ptr)
					break;
				ptr++;
			}
			break;

		case VERSION_OPTION:
			printf("candump %s\n",VERSION);
			exit(0);

		default:
			fprintf(stderr, "Unknown option %c\n", opt);
			break;
		}
	}

	if (optind == argc) {
		print_usage(basename(argv[0]));
		exit (EXIT_SUCCESS);
	}
	
	fprintf(out, "interface = %s, family = %d, type = %d, proto = %d\n",
		argv[optind], family, type, proto);
	if ((s = socket(family, type, proto)) < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strncpy(ifr.ifr_name, argv[optind], sizeof(ifr.ifr_name));
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_id = CAN_FLAG_ALL;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	if(filter) {
		if(setsockopt(s, SOL_CAN_RAW, SO_CAN_SET_FILTER, filter, filter_count * sizeof(struct can_filter)) != 0) {
			perror("setsockopt");
			exit(1);
		}
	}

	if (optdaemon)
		daemon(1, 0);
	else {
		signal(SIGTERM, sigterm);
		signal(SIGHUP, sigterm);
	}

	if (optout) {
		out = fopen(optout, "a");
		if (!out) {
			perror("fopen");
			exit (EXIT_FAILURE);
		}
	}

	while (running) {
		if ((nbytes = read(s, &frame, sizeof(struct can_frame))) < 0) {
			perror("read");
			return 1;
		} else {
			if (frame.can_id & CAN_FLAG_EXTENDED)
				n = snprintf(buf, BUF_SIZ, "<0x%08x> ", frame.can_id & CAN_ID_EXT_MASK);
			else
				n = snprintf(buf, BUF_SIZ, "<0x%03x> ", frame.can_id & CAN_ID_STD_MASK);

			n += snprintf(buf + n, BUF_SIZ - n, "[%d] ", frame.can_dlc);
			for (i = 0; i < frame.can_dlc; i++) {
				n += snprintf(buf + n, BUF_SIZ - n, "%02x ", frame.payload.data_u8[i]);
			}
			if (frame.can_id & CAN_FLAG_RTR)
				n += snprintf(buf + n, BUF_SIZ - n, "remote request");

			fprintf(out, "%s\n", buf);

			do {
				err = fflush(out);
				if (err == -1 && errno == EPIPE) {
					err = -EPIPE;
					fclose(out);
					out = fopen(optout, "a");
					if (!out)
						exit (EXIT_FAILURE);
				}
			} while (err == -EPIPE);

			n = 0;
		}
	}

	exit (EXIT_SUCCESS);
}
