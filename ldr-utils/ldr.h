/*
 * File: ldr.h
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * View LDR contents; based on the "Visual DSP++ 4.0 Loader Manual"
 * and misc Blackfin HRMs
 */

#ifndef __LDR_H__
#define __LDR_H__

#include "headers.h"
#include "helpers.h"
#include "sdp.h"

typedef struct {
	size_t offset;                /* file offset */
	void *header;                 /* target-specific block header */
	size_t header_size;           /* cache sizes for higher common code */
	void *data;                   /* buffer for block data */
	size_t data_size;             /* cache sizes for higher common code */
} BLOCK;

typedef struct {
	BLOCK *blocks;
	size_t num_blocks;
} DXE;

typedef struct {
	DXE *dxes;
	size_t num_dxes;
	void *header;                 /* for global LDR flags */
	size_t header_size;
} LDR;

typedef struct {
	size_t offset, length;
	char *filler_file;
} hole;

struct ldr_create_options {
	char *bmode;                  /* (BF53x) Desired boot mode */
	char port;                    /* (BF53x) PORT on CPU for HWAIT signals */
	unsigned int gpio;            /* (BF53x) GPIO on CPU for HWAIT signals */
	uint16_t dma;                 /* (BF54x) DMA setting */
	unsigned int flash_bits;      /* (BF56x) bit size of the flash */
	unsigned int wait_states;     /* (BF56x) number of wait states */
	unsigned int flash_holdtimes; /* (BF56x) number of hold time cycles */
	unsigned int spi_baud;        /* (BF56x) baud rate for SPI boot */
	uint32_t block_size;          /* block size to break the DXE up into */
	char *init_code;              /* initialization routine */
	hole hole;                    /* punch a hole in LDR image */
	bool use_vmas;                /* use the VMA addresses rather than LMA */
	bool jump_block;              /* create a jump block at start of L1 */
	char **filelist;
};

struct ldr_load_options {
	const char *dev;              /* path to the device load point */
	size_t baud;                  /* baud rate to send LDR */
	bool ctsrts;                  /* use hardware flow control */
	bool prompt;                  /* prompt before every communication step */
	int sleep_time;               /* time (in usecs) to sleep between blocks */
};

struct ldr_dump_options {
	const char *filename;
	bool dump_fill;
};

#include "lfd.h"

#endif
