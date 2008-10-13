/*
 * canutils/canconfig.c
 *
 * Copyright (C) 2005, 2008 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
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
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define VALUE_MAX		256
#define SYSFS_PATH_MAX		256
#define SYSFS_MNT_PATH		"/sys"
#define SYSFS_PATH_ENV		"SYSFS_PATH"

#define CLASS_NET		"/class/net/"

static char sysfs_path[SYSFS_PATH_MAX];
static const char *dev;
static int version;

enum can_sysfs_version {
	SV_0,
	SV_1,
	SV_MAX,
};

enum can_sysfs_entries {
	SE_BITRATE,
	SE_MAX,
};


static const char *can_sysfs[SV_MAX][SE_MAX] = {
	[SV_0][SE_BITRATE] = "can_bitrate",
	[SV_1][SE_BITRATE] = "can_bittiming/bitrate",
};

static void help(void)
{
	fprintf(stderr, "usage:\n\t"
		"canconfig <dev> bitrate { BR }\n\t\t"
		"BR := <bitrate in Hz>\n\t\t"
		"canconfig <dev> mode\n"
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


static void make_path(char *key_path, const char *key, size_t key_path_len)
{
	strncpy(key_path, sysfs_path, key_path_len);
	strncat(key_path, key, key_path_len);
	key_path[key_path_len - 1] = 0;
}


static ssize_t get_value(const char* key, char *value, size_t value_len)
{
	char key_path[SYSFS_PATH_MAX];
	int fd;
	ssize_t len;

	make_path(key_path, key, sizeof(key_path));

	fd = open(key_path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "opening '%s' failed: ",
			key_path);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	len = read(fd, value, value_len - 1);
	close(fd);
	if (len < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	value[len] = 0;

	/* remove trailing newline(s) */
	while (len > 0 && value[len - 1] == '\n')
		value[--len] = '\0';

	return len;
}


static ssize_t set_value(const char* key, const char *value)
{
	char key_path[SYSFS_PATH_MAX];
	int fd;
	ssize_t len;

	make_path(key_path, key, sizeof(key_path));

	fd = open(key_path, O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "opening '%s' failed: ",
			key_path);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	len = write(fd, value, strlen(value));
	close(fd);

	return len;
}



#if 0

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

#endif


static void do_show_bitrate(int argc, char* argv[])
{
	char value[VALUE_MAX];
	unsigned long bitrate;

	get_value(can_sysfs[version][SE_BITRATE], value, sizeof(value));
	bitrate = strtoul(value, NULL, 10);

	if (bitrate != 0)
		fprintf(stdout,
			"%s: bitrate %lu\n", dev, bitrate);
	else
		fprintf(stdout,
			"%s: bitrate unknown\n", dev);
}

static void do_set_bitrate(int argc, char* argv[])
{
	ssize_t err;

	err = set_value(can_sysfs[version][SE_BITRATE], argv[3]);
	if (err < 0) {
		fprintf(stderr, "setting bitrate failed:");
		perror(NULL);
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
	char value[VALUE_MAX];

	get_value("can_state", value, sizeof(value));

	fprintf(stdout, "%s\n", value);
}

static void cmd_state(int argc, char *argv[])
{
	do_show_state(argc, argv);

	exit(EXIT_SUCCESS);
}



static void do_show_ctrlmode(int argc, char *argv[])
{
	char value[VALUE_MAX];

	get_value("can_ctrlmode", value, sizeof(value));

	fprintf(stdout, "%s\n", value);
}

static void do_set_ctrlmode(int argc, char* argv[])
{
	ssize_t err;

	err = set_value("can_ctrlmode", argv[3]);
	if (err < 0) {
		fprintf(stderr, "setting ctrlmode failed:");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}

static void cmd_ctrlmode(int argc, char *argv[])
{
	if (argc >= 4) {
		do_set_ctrlmode(argc, argv);
	}
	do_show_ctrlmode(argc, argv);

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

	exit(EXIT_SUCCESS);
}


static int get_sysfs_path_try(int version)
{
	struct stat sb;
	char *sysfs_path_env;
	char entry_path[SYSFS_PATH_MAX];

	sysfs_path_env = getenv(SYSFS_PATH_ENV);

	if (sysfs_path_env != NULL) {
		size_t len;

		strncpy(sysfs_path, sysfs_path_env, sizeof(sysfs_path));

		len = strlen(sysfs_path);
		while (len > 0 && sysfs_path[len-1] == '/')
			sysfs_path[--len] = '\0';
	} else {
		strncpy(sysfs_path, SYSFS_MNT_PATH, sizeof(sysfs_path));
	}

	strncat(sysfs_path, CLASS_NET, sizeof(sysfs_path));
	strncat(sysfs_path, dev, sizeof(sysfs_path));
	strncat(sysfs_path, "/", sizeof(sysfs_path));
	sysfs_path[sizeof(sysfs_path) - 1] = 0;

	/*
	 * test if interface is present
	 * and actually a CAN interface
	 * using "can_bitrate" as indicator
	 */
	strncpy(entry_path, sysfs_path, sizeof(entry_path));
	strncat(entry_path, can_sysfs[version][SE_BITRATE], sizeof(entry_path));
	entry_path[sizeof(entry_path) - 1] = 0;

	return stat(entry_path, &sb);
}

static void get_sysfs_path(const char *_dev)
{
	int i, err;

	dev = _dev;

	for (i = 0; i < SV_MAX; i++) {
		err = get_sysfs_path_try(i);
		if (!err) {
			version = i;
			return;
		}
	}

	fprintf(stderr, "opening CAN interface '%s' in sysfs failed, "
		"maybe not a CAN interface\n",
		dev);
	perror(NULL);

	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

	if ((argc < 2) || !strcmp(argv[1], "--help"))
		help();

	get_sysfs_path(argv[1]);

	if (argc < 3)
		cmd_show_interface(argc, argv);

	if (!strcmp(argv[2], "baudrate"))
		cmd_baudrate(argc, argv);
	if (!strcmp(argv[2], "bitrate"))
		cmd_bitrate(argc, argv);
	if (!strcmp(argv[2], "mode"))
		cmd_ctrlmode(argc, argv);
#if 0
	if (!strcmp(argv[2], "setentry"))
		cmd_setentry(argc, argv);
#endif

	if (!strcmp(argv[2], "state"))
		cmd_state(argc, argv);

	help();

	exit(EXIT_SUCCESS);
}
