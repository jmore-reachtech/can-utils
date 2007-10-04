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
		"canconfig <dev> tweak [ VALs ]\n\t\t"
		"VALs := <TBR TQ ERR PPS PHS1 PHS2 SJW SAM>\n\t\t"
		" TBR bit rate to be tweaked in Hz\n\t\t"
		" TQ time quantum in ps\n\t\t"
		" ERR max. allowed error in pps\n\t\t"
		" PPS Prop_Seg in tq\n\t\t"
		" PHS1 Phase_Seg1 in tq\n\t\t"
		" PHS2 Phase_Seg2 in tq\n\t\t"
		" SJW in tq\n\t\t"
		" SAM 1 for 3 times sampling, 0 else\n\t\t"
		" without VALSs " CONFIG_FILE_NAME " will be read\n"
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

static void tweak_a_bitrate(uint32_t bit_rate, uint32_t tq, uint32_t err,
		uint32_t prop_seg, uint32_t phase_seg1, uint32_t phase_seg2,
		uint32_t sjw, int sam)
{
	struct can_bit_time_custom *bit_time = (struct can_bit_time_custom *)&ifr.ifr_ifru;
	int i;

	if (sizeof(struct can_bit_time_custom) > sizeof(ifr.ifr_ifru)) {
		printf("Error can_bit_time_custom to large! (%d, %d)",
			sizeof(struct can_bit_time_custom), sizeof(ifr.ifr_ifru));
		exit(EXIT_FAILURE);
	}

	bit_time->bit_rate = bit_rate;
	bit_time->tq = tq;
	bit_time->bit_error = err;
	bit_time->prop_seg = prop_seg;
	bit_time->phase_seg1 = phase_seg1;
	bit_time->phase_seg2 = phase_seg2;
	bit_time->sjw = sjw;
	bit_time->sam = sam;

	i = ioctl(s, SIOCSCANDEDBITTIME, &ifr);
	if (i < 0) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
}

static void tweak_from_command_line(int argc, char *argv[])
{
	uint32_t bit_rate;
	uint32_t tq;
	uint32_t err;
	uint32_t prop_seg;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	uint32_t sjw;
	int sam;

	if (argc < 9)
		help();

	bit_rate = (uint32_t)strtoul(argv[3], NULL, 0);
	tq = (uint32_t)strtoul(argv[4], NULL, 0);
	err = (uint32_t)strtoul(argv[5], NULL, 0);
	prop_seg = (uint32_t)strtoul(argv[6], NULL, 0);
	phase_seg1 = (uint32_t)strtoul(argv[7], NULL, 0);
	phase_seg2 = (uint32_t)strtoul(argv[8], NULL, 0);
	sjw = (uint32_t)strtoul(argv[9], NULL, 0);
	sam = (int)strtoul(argv[10], NULL, 0);

	tweak_a_bitrate(bit_rate, tq, err, prop_seg, phase_seg1, phase_seg2, sjw, sam);

	exit(EXIT_SUCCESS);
}

static void tweak_from_config_file(int argc, char *argv[])
{
	FILE *config_file;
	char file_name[PATH_MAX];
	int first_char,cnt,line = 1;
	uint32_t bit_rate;
	uint32_t tq;
	uint32_t err;
	uint32_t prop_seg;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	uint32_t sjw;
	int sam;

	sprintf(file_name, CONFIG_FILE_NAME_SP, argv[1]);
	config_file = fopen(file_name, "r");
	if (config_file == NULL) {
		sprintf(file_name, CONFIG_FILE_NAME);
		config_file = fopen(file_name, "r");
		if (config_file == NULL) {
			perror("Config file (" CONFIG_FILE_NAME ")");
			exit(EXIT_FAILURE);
		}
	}

	while(!feof(config_file)) {
		first_char = fgetc(config_file);
		printf("First char in line %d is: %02x (%c)\n", line, first_char, first_char);
		if (first_char == EOF)
			break;
		if (first_char == '#') {
			printf("eating line %d ", line);
			cnt=fscanf(config_file,"%*[^\n]");	/* eat the whole line */
			printf("-> %d\n", cnt);
			fgetc(config_file); /* FIXME: eat the \n */
			line ++;
			continue;
		}
		else
			ungetc(first_char, config_file);

		cnt = fscanf(config_file,"%u %u %u %u %u %u %u %d\n", &bit_rate,
			&tq, &err, &prop_seg, &phase_seg1, &phase_seg2,
			&sjw, &sam);
		if (cnt != 6) {
			fprintf(stderr,"Wrong data format in line %d in %s\n",
				line, file_name);
			exit(EXIT_FAILURE);
		}
		line ++;
		tweak_a_bitrate(bit_rate, tq, err, prop_seg, phase_seg1,
				phase_seg2, sjw, sam);
	}

	fclose(config_file);

	exit(EXIT_SUCCESS);
}

static void cmd_tweak(int argc, char *argv[])
{
	if (argc < 4)
		tweak_from_config_file(argc, argv);
	else {
		if (argc < 9)
			help();
		tweak_from_command_line(argc, argv);
	}
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
#if 1
	if ((s = socket(AF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
#endif
	if (argc < 3)
		cmd_show_interface(argc, argv);

	if (!strcmp(argv[2], "baudrate"))
		cmd_baudrate(argc, argv);
	if (!strcmp(argv[2], "mode"))
		cmd_mode(argc, argv);
	if (!strcmp(argv[2], "tweak"))
		cmd_tweak(argc, argv);
	
/* 	if (!strcmp(argv[2], "state")) */
/* 		cmd_state(argc, argv); */

	help();

	exit(EXIT_SUCCESS);
}
