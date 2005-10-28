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

#include <socket-can/can.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
				      
int             s;
struct ifreq	ifr;

struct speed_map {
	unsigned short speed;
	unsigned short value;
};

static const struct speed_map speeds[] = {
	{CAN_BAUD_10K,   10},
	{CAN_BAUD_20K,   20},
	{CAN_BAUD_50K,   50},
	{CAN_BAUD_100K, 100},
	{CAN_BAUD_125K, 125},
	{CAN_BAUD_250K, 250},
	{CAN_BAUD_500K, 500},
	{CAN_BAUD_800K, 800},
	{CAN_BAUD_1M,  1000},
};

static const int NUM_SPEEDS = (sizeof(speeds) / sizeof(struct speed_map));

unsigned long can_baud_to_value(speed_t speed)
{
	int i = 0;

	do {
		if (speed == speeds[i].speed) {
			return speeds[i].value;
		}
	} while (++i < NUM_SPEEDS);

	return 0;
}

speed_t can_value_to_baud(unsigned long value)
{
	int i = 0;

	do {
		if (value == can_baud_to_value(speeds[i].speed)) {
			return speeds[i].speed;
		}
	} while (++i < NUM_SPEEDS);

	return (speed_t)-1;
}


void help(void)
{
    fprintf(stderr, "usage:\n\t"
	    "canconfig <dev> baudrate { BR | BTR }\n\t\t"
	    "BR := { 10 | 20 | 50 | 100 | 125 | 250 | 500 | 800 | 1000 }\n\t\t"
	    "BTR := btr_sja1000 <brp> <sjw> <tseg1> <tseg2> <sam>\n\t\t"
	    "BTR := btr_c_can <brp> <sjw> <tseg1> <tseg2>\n\t\t"
	    "BTR := btr_nios <prescale> <timea> <timeb>\n\t"
	    "canconfig <dev> mode MODE\n\t\t"
	    "MODE := { start }\n\t"
	    "canconfig <dev> state\n"
	    );

    exit(EXIT_FAILURE);
}


void do_show_baudrate(int argc, char* argv[])
{
	struct can_baudrate	*br = (struct can_baudrate *)&ifr.ifr_ifru;
	int			i, value;
	
	i = ioctl(s, SIOCGCANBAUDRATE, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	switch (br->baudrate) {
		case CAN_BAUD_BTR_SJA1000:
			fprintf(stdout, "%s: sja1000 brp=%d sjw=%d tseg1=%d tseg2=%d sam=%d\n",
				ifr.ifr_name,
				br->btr.sja1000.brp,
				br->btr.sja1000.sjw,
				br->btr.sja1000.tseg1,
				br->btr.sja1000.tseg2,
				br->btr.sja1000.sam);
			break;
		case CAN_BAUD_BTR_C_CAN:
			fprintf(stdout, "%s: C-Can brp=%d sjw=%d tseg1=%d tseg2=%d\n",
				ifr.ifr_name,
				br->btr.c_can.brp,
				br->btr.c_can.sjw,
				br->btr.c_can.tseg1,
				br->btr.c_can.tseg2);
			break;
		case CAN_BAUD_BTR_NIOS:
			fprintf(stdout, "%s: nios prescale=%d timea=%d timeb=%d\n",
				ifr.ifr_name,
				br->btr.nios.prescale,
				br->btr.nios.timea,
				br->btr.nios.timeb);
			break;
		default:
			value = can_baud_to_value(br->baudrate);
		if (value != 0)
			fprintf(stdout,
				"%s: baudrate %d\n", ifr.ifr_name, value);
		else
			fprintf(stdout, "baudrate not recognized. update canconfig?\n");
		break;
	}
}


void do_set_baudrate(int argc, char* argv[])
{
	struct can_baudrate	*br = (struct can_baudrate *)&ifr.ifr_ifru;
	speed_t			baudrate;
	int			value, i;

	if (!strcmp(argv[3], "btr_sja1000")) {
		struct can_baudrate_sja1000 *br_sja1000 = &br->btr.sja1000;

		if (argc < 9)
			help();

		br->baudrate = CAN_BAUD_BTR_SJA1000;
		br_sja1000->brp = strtol(argv[4], NULL, 0);
		br_sja1000->sjw = strtol(argv[5], NULL, 0);
		br_sja1000->tseg1 = strtol(argv[6], NULL, 0);
		br_sja1000->tseg2 = strtol(argv[7], NULL, 0);
		br_sja1000->sam = strtol(argv[8], NULL, 0);

	} else if (!strcmp(argv[3], "btr_c_can")) {
		struct can_baudrate_c_can *br_c_can = &br->btr.c_can;

		if (argc < 8)
			help();

		br->baudrate = CAN_BAUD_BTR_C_CAN;
		br_c_can->brp = strtol(argv[4], NULL, 0);
		br_c_can->sjw = strtol(argv[5], NULL, 0);
		br_c_can->tseg1 = strtol(argv[6], NULL, 0);
		br_c_can->tseg2 = strtol(argv[7], NULL, 0);

	} else if (!strcmp(argv[3], "btr_nios")) {
		struct can_baudrate_nios *br_nios = &br->btr.nios;

		if (argc < 7)
			help();

		br->baudrate = CAN_BAUD_BTR_NIOS;
		br_nios->prescale = strtol(argv[4], NULL, 0);
		br_nios->timea = strtol(argv[5], NULL, 0);
		br_nios->timeb = strtol(argv[6], NULL, 0);

	} else {
		value = atoi(argv[3]);
		baudrate = can_value_to_baud(value);
		if ( baudrate == -1) {
			fprintf(stderr, "invalid baudrate\n");
			exit(EXIT_FAILURE);
		}
		br->baudrate = baudrate;
	}

	i = ioctl(s, SIOCSCANBAUDRATE, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
}


void cmd_baudrate(int argc, char *argv[])
{
	if (argc >= 4) {
		do_set_baudrate(argc, argv);
	}
	do_show_baudrate(argc, argv);

	exit(EXIT_SUCCESS);
}


void cmd_mode(int argc, char *argv[])
{
	union can_settings *settings = (union can_settings *)&ifr.ifr_ifru;
	int i;

	if (argc < 4 )
		help();

	if (!strcmp(argv[3], "start")) {
		settings->mode = CAN_MODE_START;
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


void do_show_state(int argc, char *argv[])
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


void cmd_state(int argc, char *argv[])
{
	do_show_state(argc, argv);

	exit(EXIT_SUCCESS);
}


void cmd_show_interface(int argc, char *argv[])
{
	do_show_baudrate(argc, argv);
	do_show_state(argc, argv);

	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
	if ((argc < 2) || !strcmp(argv[1], "--help"))
		help();
	
	if ((s = socket(AF_CAN, SOCK_RAW, 0)) < 0) {
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
	if (!strcmp(argv[2], "state"))
		cmd_state(argc, argv);

	help();

	return 0;
}
