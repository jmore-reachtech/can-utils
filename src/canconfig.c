/*
 * canutils/canconfig.c
 *
 * Copyright (C) 2005 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
 * Copyright (C) 2007 Juergen Beisert <jbe@pengutronix.de>, Pengutronix
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

#define kHz 1000

#define CONFIG_FILE_NAME "/etc/canconfig.conf"
#define CONFIG_FILE_NAME_SP "/etc/canconfig-%s.conf"

int             s;
struct ifreq	ifr;

static void help(void)
{
	fprintf(stderr, "usage:\n\t"
		"canconfig <dev> baudrate { BR }\n\t\t"
		"BR := <baudrate>\n\t\t"
		"canconfig <dev> mode MODE\n\t\t"
		"MODE := { start }\n\t"
		"canconfig <dev> setentry [ VALs ]\n\t\t"
		"VALs := <bitrate | tq | err | prop_seg | phase_seg1 | phase_seg2 | sjw | sam>\n\t\t"
		" bitrate <nominal bit rate to be set [Hz]>\n\t\t"
		" tq <time quantum in ns>\n\t\t"
		" err <max. allowed error in pps>\n\t\t"
		" prop_seg <no. in tq>\n\t\t"
		" phase_seg1 <no. in tq>\n\t\t"
		" phase_seg2 <no. in tq\n\t\t"
		" sjw <no. in tq>\n\t\t"
		" sam <1 | 0> 1 for 3 times sampling, 0 else\n"
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
			"%s: baudrate %d\n", ifr.ifr_name, *baudrate / kHz);
	else 
		fprintf(stdout,
			"%s: baudrate unknown\n", ifr.ifr_name);

}


static void do_set_baudrate(int argc, char* argv[])
{
	uint32_t *baudrate = (uint32_t *)&ifr.ifr_ifru;
	int i;

	*baudrate = (uint32_t)strtoul(argv[3], NULL, 0) * kHz;
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

/*
 * setup a custom bitrate for a specific interface
 * Options are:
 * canconfig <interface> setentry
 * [bitrate <number>] | [tq <number>] | [err <number>] [prop_seg <number>] |
 *     [phase_seg1 <number>] | [phase_seg2 <number>] | [sjw <number>] | [sam <number>]
 */
static void cmd_setentry(int argc, char *argv[])
{
	int done, i;
	struct can_bit_time_custom *bit_time = (struct can_bit_time_custom *)&ifr.ifr_ifru;

	/* mark each field as unintialised */
	bit_time->bit_rate = bit_time->tq = bit_time->bit_error = -1;
	bit_time->prop_seg = bit_time->phase_seg1 =
			bit_time->phase_seg2 = -1;
	bit_time->sjw = -1;
	bit_time->sam = -1;

	/* runtime testing until everything here is in mainline */
	if (sizeof(struct can_bit_time_custom) > sizeof(ifr.ifr_ifru)) {
		printf("Error can_bit_time_custom to large! (%d, %d)",
		       sizeof(struct can_bit_time_custom), sizeof(ifr.ifr_ifru));
		exit(EXIT_FAILURE);
	}

	/*
	 * Note: all values must be given. There is no default
	 * value for any one of these
	 */
	if (argc < 19)
		help();

	done = 3;

	while ((done + 1) < argc) {
		if (!strcmp(argv[done], "bitrate")) {
			bit_time->bit_rate = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "tq")) {
			bit_time->tq = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "err")) {
			bit_time->bit_error = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "prop_seg")) {
			bit_time->prop_seg = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "phase_seg1")) {
			bit_time->phase_seg1 = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "phase_seg2")) {
			bit_time->phase_seg2 = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "sjw")) {
			bit_time->sjw = (uint32_t)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
		if (!strcmp(argv[done], "sam")) {
			bit_time->sam = (int)strtoul(argv[++done], NULL, 0);
			done++;
			continue;
		}
	}

	if (bit_time->bit_rate == -1) {
		fprintf(stderr, "missing bit_rate value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->tq == -1) {
		fprintf(stderr, "missing tq value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->bit_error == -1) {
		fprintf(stderr, "missing err value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->prop_seg == (__u8)-1) {
		fprintf(stderr, "missing prop_seg value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->phase_seg1 == (__u8)-1) {
		fprintf(stderr, "missing phase_seg1 value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->phase_seg2 == (__u8)-1) {
		fprintf(stderr, "missing phase_seg2 value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->sjw == -1) {
		fprintf(stderr, "missing sjw value!\n");
		exit(EXIT_FAILURE);
	}
	if (bit_time->sam == -1) {
		fprintf(stderr, "missing sam value!\n");
		exit(EXIT_FAILURE);
	}

	i = ioctl(s, SIOCSCANDEDBITTIME, &ifr);
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
	if (!strcmp(argv[2], "setentry"))
		cmd_setentry(argc, argv);

/* 	if (!strcmp(argv[2], "state")) */
/* 		cmd_state(argc, argv); */

	help();

	exit(EXIT_SUCCESS);
}
