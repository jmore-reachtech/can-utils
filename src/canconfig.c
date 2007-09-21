/*
 * canutils/canconfig.c
 *
 * Copyright (C) 2005 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License 
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include <linux/can.h>
#include <linux/can/ioctl.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
				      
int             s;
struct ifreq	ifr;

static void help(void)
{
	fprintf(stderr, "usage:\n\t"
		"canconfig <dev> baudrate { BR }\n\t\t"
		"BR := <baudrate>\n\t\t"
		"canconfig <dev> mode MODE\n\t\t"
		"MODE := { start }\n\t"
		"canconfig <dev> state\n"
		);

	exit(EXIT_FAILURE);
}


static void do_show_baudrate(int argc, char* argv[])
{
	uint32_t *baudrate = (uint32_t *)&ifr.ifr_ifru;
	int i;
	
	i = ioctl(s, SIOCGCANBAUDRATE, &ifr);

	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	if (*baudrate != -1)
		fprintf(stdout,
			"%s: baudrate %d\n", ifr.ifr_name, *baudrate / 1000);
	else 
		fprintf(stdout,
			"%s: baudrate unknown\n", ifr.ifr_name);

}


static void do_set_baudrate(int argc, char* argv[])
{
	uint32_t *baudrate = (uint32_t *)&ifr.ifr_ifru;
	int i;

	*baudrate = (uint32_t)strtoul(argv[3], NULL, 0) * 1000;
	if (*baudrate == 0) {
		fprintf(stderr, "invalid baudrate\n");
		exit(EXIT_FAILURE);
	}

	i = ioctl(s, SIOCSCANBAUDRATE, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
}


static void cmd_baudrate(int argc, char *argv[])
{
	if (argc >= 4) {
		do_set_baudrate(argc, argv);
	}
	do_show_baudrate(argc, argv);

	exit(EXIT_SUCCESS);
}

static void cmd_mode(int argc, char *argv[])
{
	can_mode_t *mode = (can_mode_t *)&ifr.ifr_ifru;
	int i;

	if (argc < 4 )
		help();

	if (!strcmp(argv[3], "start")) {
		*mode = CAN_MODE_START;
	} else {
		help();
	}

	i = ioctl(s, SIOCSCANMODE, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
		
	exit(EXIT_SUCCESS);
}


#if 0
static void do_show_state(int argc, char *argv[])
{
	union can_settings *settings = (union can_settings *)&ifr.ifr_ifru;
	enum CAN_STATE state;
	char *str;
	int i;

	i = ioctl(s, SIOCGCANSTATE, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	state = settings->state;
	fprintf(stdout, "%s: state ", ifr.ifr_name);
	switch (state) {
	case CAN_STATE_BUS_PASSIVE:
		str = "bus passive";
		break;
	case CAN_STATE_ACTIVE:
		str = "active";
		break;
	case CAN_STATE_BUS_OFF:
		str = "bus off";
		break;
	default:
		str = "<unknown>";
	}
	fprintf(stdout, "%s \n", str);

	exit(EXIT_SUCCESS);
}


static void cmd_state(int argc, char *argv[])
{
	do_show_state(argc, argv);

	exit(EXIT_SUCCESS);
}
#endif


static void cmd_show_interface(int argc, char *argv[])
{
	do_show_baudrate(argc, argv);
/* 	do_show_state(argc, argv); */

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	if ((argc < 2) || !strcmp(argv[1], "--help"))
		help();
	
	if ((s = socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);

	if (argc < 3)
		cmd_show_interface(argc, argv);

	if (!strcmp(argv[2], "baudrate"))
		cmd_baudrate(argc, argv);
	if (!strcmp(argv[2], "mode"))
		cmd_mode(argc, argv);
/* 	if (!strcmp(argv[2], "state")) */
/* 		cmd_state(argc, argv); */

	help();

	exit(EXIT_SUCCESS);
}
