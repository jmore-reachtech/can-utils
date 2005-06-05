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

#include <socket-can/can.h>

extern int optind, opterr, optopt;

static int	s = -1;
static int	running = 1;

void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [can-interface] [Options]\n", prg);
	fprintf(stderr, "Options: -f <family> (default PF_CAN = %d)\n", PF_CAN);
	fprintf(stderr, "         -t <type>   (default SOCK_RAW = %d)\n", SOCK_RAW);
	fprintf(stderr, "         -p <proto>  (default CAN_PROTO_RAW = %d)\n", CAN_PROTO_RAW);
}

void sigterm(int signo)
{
	running = 0;
}

int main(int argc, char **argv)
{
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_PROTO_RAW;
	int opt;
	struct sockaddr_can addr;
	struct can_frame frame;
	int nbytes, i;
	struct ifreq ifr;

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);

	while ((opt = getopt(argc, argv, "f:t:p:m:v:")) != -1) {
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
	strncpy(ifr.ifr_name, argv[optind], sizeof(ifr.ifr_name));
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_id = CAN_FLAG_ALL;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	while (running) {
		if ((nbytes = read(s, &frame, sizeof(struct can_frame))) < 0) {
			perror("read");
			return 1;
		} else {
			if (frame.can_id & CAN_FLAG_EXTENDED)
				printf("<0x%08x> ", frame.can_id & CAN_ID_EXT_MASK);
			else
				printf("<0x%03x> ", frame.can_id & CAN_ID_STD_MASK);

			printf("[%d] ", frame.can_dlc);
			for (i = 0; i < frame.can_dlc; i++) {
				printf("%02x ", frame.payload.data[i]);
			}
			if (frame.can_id & CAN_FLAG_RTR)
				printf("remote request");
			printf("\n");
		}
	}

	return 0;
}
