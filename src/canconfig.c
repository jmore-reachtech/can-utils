/*
 * canutils/canconfig.c
 *
 * Copyright (C) 2005, 2008 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
 * Copyright (C) 2007 Juergen Beisert <jbe@pengutronix.de>, Pengutronix
 * Copyright (C) 2009 Luotao Fu <l.fu@pengutronix.de>, Pengutronix
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
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <socketcan_netlink.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

const char *can_modes[CAN_STATE_MAX] = {
	"ACTIVE",
	"WARNING",
	"PASSIVE",
	"BUS OFF",
	"STOPPED",
	"SLEEPING"
};

static void help(void)
{
	fprintf(stderr, "usage:\n\t"
		"canconfig <dev> bitrate { BR }\n\t\t"
		"BR := <bitrate in Hz>\n\t"
		"canconfig <dev> restart-ms { RESTART_MS }\n\t\t"
		"RESTART_MS := <autorestart interval in ms>\n\t"
		"canconfig <dev> mode\n\t"
		"canconfig <dev> restart\n"
#if 0
		"MODE\n\t\t"
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
#endif
		);

	exit(EXIT_FAILURE);
}

static void do_show_bitrate(int argc, char* argv[])
{
	struct can_bittiming bt;

	if (scan_get_bittiming(argv[1], &bt) < 0)
		fprintf(stdout, "%s: bitrate unknown\n", argv[1]);
	else
		fprintf(stdout,
			"%s bitrate: %u\n", argv[1], bt.bitrate);
}

static void do_set_bitrate(int argc, char* argv[])
{
	if (scan_set_bitrate(argv[1], (__u32)strtoul(argv[3], NULL, 10)) < 0) {
		fprintf(stderr, "failed to set bitrate of %s to %lu\n",
		       	argv[1], strtoul(argv[3], NULL, 10));
		exit(EXIT_FAILURE);
	}
}

static void cmd_bitrate(int argc, char *argv[])
{
	if (argc >= 4) {
		do_set_bitrate(argc, argv);
	}
	do_show_bitrate(argc, argv);

	exit(EXIT_SUCCESS);
}

static void do_show_state(int argc, char *argv[])
{
	int state;

	if (scan_get_state(argv[1], &state) < 0) {
		fprintf(stderr, "%s: failed to get state \n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (state >= 0 && state < CAN_STATE_MAX)
		fprintf(stdout, "%s state: %s\n", argv[1], can_modes[state]);
	else
		fprintf(stderr, "%s: unknown state\n", argv[1]);
}

static void cmd_state(int argc, char *argv[])
{
	do_show_state(argc, argv);

	exit(EXIT_SUCCESS);
}

static inline void print_ctrlmode(__u32 cm_mask, __u32 cm_flag)
{
	__u32 active_cm = cm_mask & cm_flag;
	fprintf(stdout, "loopback [%s], listenonly [%s], 3 samples [%s]\n",
		(active_cm & CAN_CTRLMODE_LOOPBACK) ? "on" : "off",
		(active_cm & CAN_CTRLMODE_LISTENONLY) ? "on" : "off",
		(active_cm & CAN_CTRLMODE_3_SAMPLES) ? "on" : "off");
}

static void do_restart(int argc, char *argv[])
{
	if (scan_set_restart(argv[1]) < 0) {
		fprintf(stderr, "%s: failed to restart\n", argv[1]);
		exit(EXIT_FAILURE);
	} else {
		fprintf(stdout, "%s restarted\n", argv[1]);
	}
}

static void cmd_restart(int argc, char *argv[])
{
	do_restart(argc, argv);

	exit(EXIT_SUCCESS);
}

static void do_show_ctrlmode(int argc, char *argv[])
{
	struct can_ctrlmode cm;
	
	if (scan_get_ctrlmode(argv[1], &cm) < 0) {
		fprintf(stderr, "%s: failed to get controlmode\n", argv[1]);
		exit(EXIT_FAILURE);
	} else {
		fprintf(stdout, "%s mode: ", argv[1]);
		print_ctrlmode(cm.mask, cm.flags);
	}
}

#if 0
static void do_set_ctrlmode(int argc, char* argv[])
{
}
#endif

static void cmd_ctrlmode(int argc, char *argv[])
{
#if 0
	if (argc >= 4) {
		do_set_ctrlmode(argc, argv);
	}
#endif
	do_show_ctrlmode(argc, argv);

	exit(EXIT_SUCCESS);
}

static void do_show_restart_ms(int argc, char* argv[])
{
	__u32 restart_ms;

	if (scan_get_restart_ms(argv[1], &restart_ms) < 0) {
		fprintf(stderr, "%s: failed to get restart_ms\n", argv[1]);
		exit(EXIT_FAILURE);
	} else
		fprintf(stdout,
			"%s restart_ms: %u\n", argv[1], restart_ms);
}

static void do_set_restart_ms(int argc, char* argv[])
{
	if (scan_set_restart_ms(argv[1],
				(__u32)strtoul(argv[3], NULL, 10)) < 0) {
		fprintf(stderr, "failed to set restart_ms of %s to %lu\n",
		       	argv[1], strtoul(argv[3], NULL, 10));
		exit(EXIT_FAILURE);
	}
}

static void cmd_restart_ms(int argc, char *argv[])
{
	if (argc >= 4) {
		do_set_restart_ms(argc, argv);
	}

	do_show_restart_ms(argc, argv);

	exit(EXIT_SUCCESS);
}

static void cmd_baudrate(int argc, char *argv[])
{
	fprintf(stderr, "%s: baudrate is deprecated, pleae use bitrate\n",
		argv[0]);

	exit(EXIT_FAILURE);
}

static void cmd_show_interface(int argc, char *argv[])
{
	do_show_bitrate(argc, argv);
	do_show_state(argc, argv);
	do_show_ctrlmode(argc, argv);
	do_show_restart_ms(argc, argv);

	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{

	if ((argc < 2) || !strcmp(argv[1], "--help"))
		help();

	if (argc < 3)
		cmd_show_interface(argc, argv);

	if (!strcmp(argv[2], "baudrate"))
		cmd_baudrate(argc, argv);
	if (!strcmp(argv[2], "bitrate"))
		cmd_bitrate(argc, argv);
	if (!strcmp(argv[2], "mode"))
		cmd_ctrlmode(argc, argv);
	if (!strcmp(argv[2], "restart"))
		cmd_restart(argc, argv);
	if (!strcmp(argv[2], "restart-ms"))
		cmd_restart_ms(argc, argv);

#if 0
	if (!strcmp(argv[2], "setentry"))
		cmd_setentry(argc, argv);
#endif

	if (!strcmp(argv[2], "state"))
		cmd_state(argc, argv);

	help();

	exit(EXIT_SUCCESS);
}
