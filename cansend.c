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
static int	running = 1;

void sigterm(int signo)
{
	running = 0;
}

void print_usage(char *prg)
{
        fprintf(stderr, "Usage: %s -i [can-interface] [Options]\n", prg);
        fprintf(stderr, "Options: -f <family> (default PF_CAN = %d)\n", PF_CAN);
        fprintf(stderr, "         -t <type>   (default SOCK_RAW = %d)\n", SOCK_RAW);
        fprintf(stderr, "         -p <proto>  (default CAN_PROTO_RAW = %d)\n", CAN_PROTO_RAW);
        fprintf(stderr, "         -v          (verbose)\n");
}

int main(int argc, char **argv)
{
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_PROTO_RAW;
	struct sockaddr_can addr;
	int s, opt, ret, i;
	struct can_frame frame;
	int verbose = 0;
	int num = 0;
	struct ifreq ifr;

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);

	while ((opt = getopt(argc, argv, "f:t:p:vi:")) != -1) {
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
			
			case 'i':
				frame.can_id = atoi(optarg);

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
	
	if (argv[optind] == NULL) {
		fprintf(stderr, "No Interface supplied\n");
		exit(-1);
	}

	printf("interface = %s, family = %d, type = %d, proto = %d\n",
	       argv[optind], family, type, proto);
	if ((s = socket(family, type, proto)) < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strcpy(ifr.ifr_name, argv[optind]);
	ret = ioctl(s, SIOCGIFINDEX, &ifr);
	printf("ioctl: %d ", ret);
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_id = frame.can_id;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (argv[optind+1] != NULL) {
		frame.can_dlc = strlen(argv[optind+1]);
		frame.can_dlc = frame.can_dlc > 8 ? 8 : frame.can_dlc;
		strncpy(frame.payload.data, argv[optind+1],8);
	} else {
		frame.can_dlc = 0;
	}

	printf("id: %d ",frame.can_id);
	printf("dlc: %d\n",frame.can_dlc);
	for(i = 0; i < frame.can_dlc; i++)
		printf("0x%02x ",frame.payload.data[i]);
	printf("\n");

	num = 1;
	while (num--) {
		ret = write(s, &frame, sizeof(frame));
	}

	close(s);
	return 0;
}
