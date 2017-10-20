/*
 * File: blackfin_defines.h
 *
 * Copyright 2006-2007 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Misc defines ripped out of Blackfin headers.
 */

#ifndef __BLACKFIN_DEFINES__
#define __BLACKFIN_DEFINES__

/* common stuff */
#define LO(x) ((x) & 0xFFFF)
#define HI(x) (((x) >> 16) & 0xFFFF)

#define GET_1ST_NIBBLE(x) ((x & 0x000000FF) >> 0)
#define GET_2ND_NIBBLE(x) ((x & 0x0000FF00) >> 8)
#define GET_3RD_NIBBLE(x) ((x & 0x00FF0000) >> 16)
#define GET_4TH_NIBBLE(x) ((x & 0xFF000000) >> 24)

#define FILL_ADDR_16(var, val, idx1, idx2) \
	do { \
		var[idx1] = GET_1ST_NIBBLE(val); \
		var[idx2] = GET_2ND_NIBBLE(val); \
	} while (0)
#define FILL_ADDR_32(var, val, idx1, idx2, idx3, idx4) \
	do { \
		var[idx1] = GET_1ST_NIBBLE(val); \
		var[idx2] = GET_2ND_NIBBLE(val); \
		var[idx3] = GET_3RD_NIBBLE(val); \
		var[idx4] = GET_4TH_NIBBLE(val); \
	} while (0)

#endif
