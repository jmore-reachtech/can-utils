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
};

void print_usage(char *prg)
{
        fprintf(stderr, "Usage: %s <can-interface> [Options]\n"
                        "Options:\n"
	                " -f, --family=FAMILY   Protocol family (default PF_CAN = %d)\n"
                        " -t, --type=TYPE       Socket type, see man 2 socket (default SOCK_RAW = %d)\n"
                        " -p, --protocol=PROTO  CAN protocol (default CAN_PROTO_RAW = %d)\n"
                        " -v, --verbose         be verbose\n"
			"     --version         print version information and exit\n",
				prg, PF_CAN, SOCK_RAW, CAN_PROTO_RAW);
}

void sigterm(int signo)
{
	printf("got signal %d\n", signo);
	running = 0;
}

int main(int argc, char **argv)
{
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_PROTO_RAW;
	int opt;
	struct sockaddr_can addr;
	struct can_frame frame;
	int nbytes, i;
	int verbose = 0;
	struct ifreq ifr;

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);

	struct option		long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "family", required_argument, 0, 'f' },
		{ "protocol", required_argument, 0, 'p' },
		{ "type", required_argument, 0, 't' },
		{ "version", no_argument, 0, VERSION_OPTION},
		{ "verbose", no_argument, 0, 'v'},
		{ 0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "hf:t:p:v", long_options, NULL)) != -1) {
		switch (opt) {
			case 'f':
				family = atoi(optarg);
				break;

			case 't':
				type = atoi(optarg);
				break;

			case 'p':
				proto = atoi(optarg);
				break;

			case 'v':
				verbose = 1;
				break;

			case 'h':
				print_usage(basename(argv[0]));
				exit(0);

			case VERSION_OPTION:
				printf("canecho %s\n",VERSION);
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
	
	printf("interface = %s, family = %d, type = %d, proto = %d\n",
			argv[optind], family, type, proto);
	if ((s = socket(family, type, proto)) < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strcpy(ifr.ifr_name, argv[optind]);
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_id = CAN_FLAG_ALL;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	while (running) {
		if ((nbytes = read(s, &frame, sizeof(frame))) < 0) {
			perror("read");
			return 1;
		}
		if (verbose) {
			printf("%04x: ", frame.can_id);
			if (frame.can_id & CAN_FLAG_RTR) {
				printf("remote request");
			} else {
				printf("[%d]", frame.can_dlc);
				for (i = 0; i < frame.can_dlc; i++) {
					printf(" %02x", frame.payload.data[i]);
				}
			}
			printf("\n");
		}
		frame.can_id++;
		write(s, &frame, sizeof(frame));
	}

	return 0;
}
