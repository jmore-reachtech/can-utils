#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "can.h"

extern int optind, opterr, optopt;

static int	s = -1;
static int	running = 1;

void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [can-interface]\n", prg);
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

	while ((opt = getopt(argc, argv, "f:t:p:v")) != -1) {
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

			case '?':
				break;

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
					printf(" %02x", frame.payload.byte[i]);
				}
			}
			printf("\n");
		}
		frame.can_id++;
		write(s, &frame, sizeof(frame));
	}

	return 0;
}
