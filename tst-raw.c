#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "can.h"

int main(int argc, char **argv)
{
    int s;
    int family = PF_CAN, type = SOCK_RAW, proto = CAN_PROTO_RAW;
    int opt;
    struct sockaddr_can addr;
    unsigned char buf[4096];
    int nbytes, i;
    struct ifreq ifr;

    struct iovec bufs[] = {
	{ "abc", 4 },
	{ "def", 4 },
	{ "0123456", 8 },
	{ "xyz", 4 },
    };

    while ((opt = getopt(argc, argv, "f:t:p:")) != -1) {
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
	default:
	    fprintf(stderr, "Unknown option %c\n", opt);
	    break;
	}
    }

    printf("family = %d, type = %d, proto = %d\n", family, type, proto);
    if ((s = socket(family, type, proto)) < 0) {
	perror("socket");
	return 1;
    }

    strcpy(ifr.ifr_name, "vcan2");
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_family = family;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("bind");
	return 1;
    }
#if 0
    if (write(s, "abc", 4) < 0) {
	perror("write");
	return 1;
    }
#endif
    if (writev(s, bufs, sizeof(bufs)/sizeof(*bufs)) < 0) {
	perror("writev");
	return 1;
    }
    if ((nbytes = read(s, buf, 4096)) < 0) {
	perror("read");
	return 1;
    } else {
	printf("%d bytes read\n", nbytes);
	for (i = 0; i < nbytes; i++) {
	    printf(" %02x", buf[i]);
	    if (i % 16 == 15 || i == nbytes - 1)
		putchar('\n');
	}
    }    

    return 0;
}
