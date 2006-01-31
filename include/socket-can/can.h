/*
 * can.h (userspace)
 *
 * Copyright (C) 2004, 2005
 *
 * - Robert Schwebel, Benedikt Spranger, Marc Kleine-Budde, Pengutronix
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

#ifndef _SOCKET_CAN_CAN_H
#define _SOCKET_CAN_CAN_H

#include <stdint.h>

#define CAN_ID_EXT_MASK   0x1FFFFFFF	/* contains CAN id */
#define CAN_ID_STD_MASK   0x000007FF
#define CAN_FLAG_ALL      0x20000000	/* filter all messages */
#define CAN_FLAG_RTR      0x40000000	/* remote transmission flag*/
#define CAN_FLAG_EXTENDED 0x80000000	/* extended frame */

#define SIOCSCANBAUDRATE	(SIOCPROTOPRIVATE + 0)
#define SIOCGCANBAUDRATE	(SIOCPROTOPRIVATE + 1)
#define SIOCSCANMODE		(SIOCPROTOPRIVATE + 2)
#define SIOCGCANSTATE		(SIOCPROTOPRIVATE + 3)


enum CAN_BAUD {
   /*!
   ** Baudrate 10 kBit/sec
   */
   CAN_BAUD_10K = 0,

   /*!
   ** Baudrate 20 kBit/sec
   */
   CAN_BAUD_20K,

   /*!
   ** Baudrate 50 kBit/sec
   */
   CAN_BAUD_50K,

   /*!
   ** Baudrate 100 kBit/sec
   */
   CAN_BAUD_100K,

   /*!
   ** Baudrate 125 kBit/sec
   */
   CAN_BAUD_125K,

   /*!
   ** Baudrate 250 kBit/sec
   */
   CAN_BAUD_250K,

   /*!
   ** Baudrate 500 kBit/sec
   */
   CAN_BAUD_500K,

   /*!
   ** Baudrate 800 kBit/sec
   */
   CAN_BAUD_800K,

   /*!
   ** Baudrate 1 MBit/sec
   */
   CAN_BAUD_1M,

   /*!
   ** Max value, beside CAN_BAUD_BTR_*
   */
   CAN_BAUD_MAX,
   CAN_BAUD_UNCONFIGURED,

   CAN_BAUD_BTR_SJA1000 = 0x80,
   CAN_BAUD_BTR_C_CAN,
   CAN_BAUD_BTR_NIOS,
};


enum CAN_MODE {
   /*!   Set controller in Stop mode (no reception / transmission possible)
   */
   CAN_MODE_STOP = 0,

   /*!   Set controller into normal operation
   */
   CAN_MODE_START,

   /*!	Start Autobaud Detection
   */
   CAN_MODE_AUTO_BAUD,
   /*!	Set controller into Sleep mode
   */
   CAN_MODE_SLEEP
};


enum CAN_STATE {
	/*!
	**	CAN controller is active, no errors
	*/
	CAN_STATE_ACTIVE    = 0,

	/*!
	**	CAN controller is in stopped mode
	*/
	CAN_STATE_STOPPED,

	/*!
	**	CAN controller is in Sleep mode
	*/
	CAN_STATE_SLEEPING,

	/*!
	**	CAN controller is active, warning level is reached
	*/
	CAN_STATE_BUS_WARN  = 6,

 	/*!
	**	CAN controller is error passive
	*/
	CAN_STATE_BUS_PASSIVE,

	/*!
	**	CAN controller went into Bus Off
	*/
	CAN_STATE_BUS_OFF,

	/*!
	**	General failure of physical layer detected (if supported by hardware)
	*/
	CAN_STATE_PHY_FAULT = 10,

	/*!
	**	Fault on CAN-H detected (Low Speed CAN)
	*/
	CAN_STATE_PHY_H,

	/*!
	**	Fault on CAN-L detected (Low Speed CAN)
	*/
	CAN_STATE_PHY_L,

	CAN_STATE_ERR_BIT   = 0x10,
	CAN_STATE_ERR_STUFF = 0x20,
	CAN_STATE_ERR_FORM  = 0x30,
	CAN_STATE_ERR_CRC   = 0x40,
	CAN_STATE_ERR_ACK   = 0x50
};

struct can_baudrate_sja1000 {
	uint8_t	brp;
	uint8_t	sjw;
	uint8_t	tseg1;
	uint8_t	tseg2;
	uint8_t	sam;
};

struct can_baudrate_c_can {
	uint8_t	brp;
	uint8_t	sjw;
	uint8_t	tseg1;
	uint8_t	tseg2;
};

struct can_baudrate_nios {
	uint8_t	prescale;
	uint8_t	timea;
	uint8_t	timeb;
};

struct can_baudrate {
	enum CAN_BAUD	baudrate;
	union {
		struct can_baudrate_sja1000 sja1000;
		struct can_baudrate_c_can c_can;
		struct can_baudrate_nios nios;
	} btr;
};


union can_settings {
	enum CAN_MODE	mode;
	enum CAN_STATE	state;
};


#define AF_CAN		30	/* CAN Bus                      */
#define PF_CAN		AF_CAN

enum CAN_PROTO {
	CAN_PROTO_RAW,
	CAN_PROTO_MAX,
};

struct sockaddr_can {
    sa_family_t  can_family;
    int          can_ifindex;
    int          can_id;
};

struct can_frame {
	int	can_id;
	int	can_dlc;
	union {
                int64_t		data_64;
                int32_t		data_32[2];
                int16_t		data_16[4];
                int8_t		data_8[8];
		uint64_t	data_u64;
		uint32_t	data_u32[2];
		uint16_t	data_u16[4];
		uint8_t		data_u8[8];
		int8_t		data[8]; 		/* shortcut */
	} payload;
};

struct can_filter {
	uint32_t	can_id;
	uint32_t	can_mask;
};
#define SO_CAN_SET_FILTER 1
#define SO_CAN_UNSET_FILTER 2

#define SOL_CAN_BASE	100
#define SOL_CAN_RAW	(SOL_CAN_BASE + CAN_PROTO_RAW)

#endif /* !_SOCKET_CAN_CAN_H */
