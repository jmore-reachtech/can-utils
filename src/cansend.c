#include <can_config.h>

#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <poll.h>

#include <linux/can.h>
#include <linux/can/raw.h>

extern int optind, opterr, optopt;

static void print_usage(char *prg)
{
	fprintf(stderr,
		"Usage: %s [<can-interface>] [Options] <can-msg>\n"
		"<can-msg> can consist of up to 8 bytes given as a space separated list\n"
		"Options:\n"
		" -i, --identifier=ID	CAN Identifier (default = 1)\n"
		" -r  --rtr		send remote request\n"
		" -e  --extended	send extended frame\n"
		" -l			send message infinite times\n"
		"     --loop=COUNT	send message COUNT times\n"
		" -p  --poll		use poll(2) to wait for buffer space while sending\n"
		" -v, --verbose		be verbose\n"
		" -h, --help		this help\n"
		"     --version		print version information and exit\n",
		prg);
}

enum {
		VERSION_OPTION = CHAR_MAX + 1,
};

int main(int argc, char **argv)
{
	struct can_frame frame = {
		.can_id = 1,
	};
	struct ifreq ifr;
	struct sockaddr_can addr;
	char *interface;
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
	int loopcount = 1, infinite = 0;
	int s, opt, ret, i, dlc = 0, rtr = 0, extended = 0;
	ssize_t len;
	int use_poll = 0;
	int verbose = 0;

	struct option long_options[] = {
		{ "help",	no_argument,		0, 'h' },
		{ "poll",	no_argument,		0, 'p'},
		{ "identifier",	required_argument,	0, 'i' },
		{ "rtr",	no_argument,		0, 'r' },
		{ "extended",	no_argument,		0, 'e' },
		{ "version",	no_argument,		0, VERSION_OPTION},
		{ "verbose",	no_argument,		0, 'v'},
		{ "loop",	required_argument,	0, 'l'},
		{ 0,		0,			0, 0 },
	};

	while ((opt = getopt_long(argc, argv, "hpvi:lre", long_options, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_usage(basename(argv[0]));
			exit(0);

		case 'p':
			use_poll = 1;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'l':
			if (optarg)
				loopcount = strtoul(optarg, NULL, 0);
			else
				infinite = 1;
			break;
		case 'i':
			frame.can_id = strtoul(optarg, NULL, 0);
			break;

		case 'r':
			rtr = 1;
			break;

		case 'e':
			extended = 1;
			break;

		case VERSION_OPTION:
			printf("cansend %s\n", VERSION);
			exit(0);
		default:
			fprintf(stderr, "Unknown option %c\n", opt);
			break;
		}
	}

	if (optind == argc) {
		print_usage(basename(argv[0]));
		exit(0);
	}

	if (argv[optind] == NULL) {
		fprintf(stderr, "No Interface supplied\n");
		exit(-1);
	}
	interface = argv[optind];

	printf("interface = %s, family = %d, type = %d, proto = %d\n",
	       interface, family, type, proto);

	s = socket(family, type, proto);
	if (s < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strcpy(ifr.ifr_name, interface);
	if (ioctl(s, SIOCGIFINDEX, &ifr)) {
		perror("ioctl");
		return 1;
	}
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	for (i = optind + 1; i < argc; i++) {
		frame.data[dlc] = strtoul(argv[i], NULL, 0);
		dlc++;
		if (dlc == 8)
			break;
	}
	frame.can_dlc = dlc;


	if (extended) {
		frame.can_id &= CAN_EFF_MASK;
		frame.can_id |= CAN_EFF_FLAG;
	} else {
		frame.can_id &= CAN_SFF_MASK;
	}

	if (rtr)
		frame.can_id |= CAN_RTR_FLAG;

	if (verbose) {
		printf("id: %d ", frame.can_id);
		printf("dlc: %d\n", frame.can_dlc);
		for (i = 0; i < frame.can_dlc; i++)
			printf("0x%02x ", frame.data[i]);
		printf("\n");
	}

	while (infinite || loopcount--) {
	again:
		len = write(s, &frame, sizeof(frame));
		if (len == -1) {
			switch (errno) {
			case ENOBUFS: {
				struct pollfd fds = {
					.fd = s,
					.events = POLLOUT,
				};

				if (!use_poll) {
					perror("write");
					exit(EXIT_FAILURE);
				}

				ret = poll(&fds, 1, 1000);
				if (ret == -1 && errno != -EINTR) {
					perror("poll()");
					exit(EXIT_FAILURE);
				}
			}
			case EINTR:	/* fallthrough */
				goto again;
			default:
				perror("write");
				exit(EXIT_FAILURE);
			}
		}
	}

	close(s);
	return 0;
}
