/*
 * File: lfd_bf537.c
 *
 * Copyright 2006-2011 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Format handlers for LDR files on the BF53[467].
 */

#define __LFD_INTERNAL
#include "ldr.h"

/* 10-byte block header; See page 19-13 of BF537 HRM */
#define LDR_BLOCK_HEADER_LEN (10)
typedef struct {
	uint8_t raw[LDR_BLOCK_HEADER_LEN];        /* buffer for following members ... needs to be first */
	uint32_t target_address;                  /* blackfin memory address to load block */
	uint32_t byte_count;                      /* number of bytes in block */
	uint16_t flags;                           /* flags to control behavior */
} BLOCK_HEADER;

/* block flags; See page 19-14 of BF537 HRM */
#define LDR_FLAG_ZEROFILL    0x0001
#define LDR_FLAG_RESVECT     0x0002
#define LDR_FLAG_INIT        0x0008
#define LDR_FLAG_IGNORE      0x0010
#define LDR_FLAG_PPORT_MASK  0x0600
#define LDR_FLAG_PPORT_NONE  0x0000
#define LDR_FLAG_PPORT_PORTF 0x0200
#define LDR_FLAG_PPORT_PORTG 0x0400
#define LDR_FLAG_PPORT_PORTH 0x0600
#define LDR_FLAG_PFLAG_MASK  0x01E0
#define LDR_FLAG_PFLAG_SHIFT 5
#define LDR_FLAG_COMPRESSED  0x2000
#define LDR_FLAG_FINAL       0x8000

static const struct lfd_flag bf537_lfd_flags[] = {
	{ LDR_FLAG_ZEROFILL,    "zerofill"  },
	{ LDR_FLAG_RESVECT,     "resvect"   },
	{ LDR_FLAG_INIT,        "init"      },
	{ LDR_FLAG_IGNORE,      "ignore"    },
	{ LDR_FLAG_PPORT_NONE,  "port_none" },
	{ LDR_FLAG_PPORT_PORTF, "portf"     },
	{ LDR_FLAG_PPORT_PORTG, "portg"     },
	{ LDR_FLAG_PPORT_PORTH, "porth"     },
	{ LDR_FLAG_COMPRESSED,  "compressed"},
	{ LDR_FLAG_FINAL,       "final"     },
	{ 0, 0 }
};

/**
 *	bf53x_lfd_read_block_header - read in the BF53x block header
 *
 * The format of each block header:
 * [4 bytes for address]
 * [4 bytes for byte count]
 * [2 bytes for flags]
 */
void *bf53x_lfd_read_block_header(LFD *alfd, bool *ignore, bool *fill, bool *final, size_t *header_len, size_t *data_len)
{
	FILE *fp = alfd->fp;
	BLOCK_HEADER *header = xmalloc(sizeof(*header));
	if (fread(header->raw, 1, LDR_BLOCK_HEADER_LEN, fp) != LDR_BLOCK_HEADER_LEN)
		return NULL;
	memcpy(&(header->target_address), header->raw, sizeof(header->target_address));
	memcpy(&(header->byte_count), header->raw+4, sizeof(header->byte_count));
	memcpy(&(header->flags), header->raw+8, sizeof(header->flags));
	ldr_make_little_endian_32(header->target_address);
	ldr_make_little_endian_32(header->byte_count);
	ldr_make_little_endian_16(header->flags);
	*ignore = !!(header->flags & LDR_FLAG_IGNORE);
	*fill = !!(header->flags & LDR_FLAG_ZEROFILL);
	*final = !!(header->flags & LDR_FLAG_FINAL);
	*header_len = LDR_BLOCK_HEADER_LEN;
	*data_len = header->byte_count;
	return header;
}

bool bf53x_lfd_display_dxe(LFD *alfd, size_t d)
{
	LDR *ldr = alfd->ldr;
	size_t i, b;
	uint16_t hflags, flags, ignore_flags;

	/* since we let the BF533/BF561 LFDs jump here, let's mask some
	 * flags that aren't used for those targets ...
	 */
	ignore_flags = family_is(alfd, "BF537") ? 0 : LDR_FLAG_PPORT_MASK;

	if (quiet)
		printf("              Offset      Address     Bytes    Flags\n");
	for (b = 0; b < ldr->dxes[d].num_blocks; ++b) {
		BLOCK *block = &(ldr->dxes[d].blocks[b]);
		BLOCK_HEADER *header = block->header;
		if (quiet)
			printf("    Block %2zu 0x%08zX: ", b+1, block->offset);
		else
			printf("    Block %2zu at 0x%08zX\n", b+1, block->offset);

		if (quiet) {
			printf("0x%08X 0x%08X 0x%04X ( ", header->target_address, header->byte_count, header->flags);
		} else if (verbose) {
			printf("\t\tTarget Address: 0x%08X ( %s )\n", header->target_address,
				(header->target_address > 0xFF000000 ? "L1" : "SDRAM"));
			printf("\t\t    Byte Count: 0x%08X ( %u bytes )\n", header->byte_count, header->byte_count);
			printf("\t\t         Flags: 0x%04X     ( ", header->flags);
		} else {
			printf("         Addr: 0x%08X Bytes: 0x%08X Flags: 0x%04X ( ",
				header->target_address, header->byte_count, header->flags);
		}

		hflags = header->flags & ~ignore_flags;

		flags = hflags & LDR_FLAG_PPORT_MASK;
		if (flags)
			for (i = 0; bf537_lfd_flags[i].desc; ++i)
				if (flags == bf537_lfd_flags[i].flag)
					printf("%s ", bf537_lfd_flags[i].desc);
		flags = (hflags & LDR_FLAG_PFLAG_MASK) >> LDR_FLAG_PFLAG_SHIFT;
		if (flags)
			printf("gpio%i ", flags);

		flags = hflags & ~LDR_FLAG_PPORT_MASK;
		for (i = 0; bf537_lfd_flags[i].desc; ++i)
			if (flags & bf537_lfd_flags[i].flag)
				printf("%s ", bf537_lfd_flags[i].desc);

		printf(")\n");
	}

	return true;
}

/*
 * ldr_create()
 */
static bool _bf53x_lfd_write_header(FILE *fp, uint16_t flags,
                                    uint32_t addr, uint32_t count)
{
	ldr_make_little_endian_32(addr);
	ldr_make_little_endian_32(count);
	ldr_make_little_endian_16(flags);
	return
		fwrite(&addr, sizeof(addr), 1, fp) == 1 &&
		fwrite(&count, sizeof(count), 1, fp) == 1 &&
		fwrite(&flags, sizeof(flags), 1, fp) == 1;
}

static void _bf53x_lfd_block_flags(LFD *alfd, const struct ldr_create_options *opts,
                                   uint16_t *flags, uint32_t *addr)
{
	*flags = 0;
	if (!target_is(alfd, "BF531") &&
	    !target_is(alfd, "BF532"))
		*flags |= LDR_FLAG_RESVECT;

	*flags |= (opts->gpio << LDR_FLAG_PFLAG_SHIFT) & LDR_FLAG_PFLAG_MASK;
	if (family_is(alfd, "BF537")) {
		switch (toupper(opts->port)) {
			case 'F': *flags |= LDR_FLAG_PPORT_PORTF; break;
			case 'G': *flags |= LDR_FLAG_PPORT_PORTG; break;
			case 'H': *flags |= LDR_FLAG_PPORT_PORTH; break;
			default:  *flags |= LDR_FLAG_PPORT_NONE; break;
		}
	}

	*addr = (*flags & LDR_FLAG_RESVECT ? 0xFFA00000 : 0xFFA08000);
}

bool bf53x_lfd_write_ldr(LFD *alfd, const void *void_opts)
{
	const struct ldr_create_options *opts = void_opts;
	FILE *fp = alfd->fp;
	uint16_t flags;
	uint32_t addr, count;

	/* This dummy block is only needed in flash/fifo boot modes */
	if (opts->bmode == NULL ||
	    (strcasecmp(opts->bmode, "parallel") &&
	     strcasecmp(opts->bmode, "para") &&
	     strcasecmp(opts->bmode, "fifo")))
		return true;

	/* Seed the initial flags/addr values */
	_bf53x_lfd_block_flags(alfd, opts, &flags, &addr);

	/* The address field doubles up as extra flag bits:
	 * 0x40: 8-bit flash, no dma
	 * 0x60: 16-bit flash, 8-bit dma
	 * 0x20: 16-bit flash, 16-bit dma
	 */
	if (opts->flash_bits == 16) {
		if (opts->dma == 8)
			addr |= 0x60;
		else
			addr |= 0x20;
	} else
		addr |= 0x40;

	/* the manual says to use 0xCCCCCCCC but it, like the cake, is a lie */
	count = 0;

	/* make sure this block gets ignored */
	flags |= LDR_FLAG_IGNORE;

	return _bf53x_lfd_write_header(fp, flags, addr, count);
}

bool bf53x_lfd_write_block(LFD *alfd, uint8_t dxe_flags,
                           const void *void_opts, uint32_t addr,
                           uint32_t count, void *src)
{
	const struct ldr_create_options *opts = void_opts;
	FILE *fp = alfd->fp;
	uint16_t flags;
	uint32_t default_init_addr;

	/* Seed the initial flags/addr values */
	_bf53x_lfd_block_flags(alfd, opts, &flags, &default_init_addr);

	/* we dont need a special first ignore block */
	if (dxe_flags & DXE_BLOCK_FIRST)
		return true;
	if (dxe_flags & DXE_BLOCK_INIT) {
		flags |= LDR_FLAG_INIT;
		addr = default_init_addr;
	}
	if (dxe_flags & DXE_BLOCK_JUMP) {
		/* if the ELF's entry is already at the
		 * expected location, then we can omit it.
		 * XXX: we can ignore the case where the entry overlays the
		 *      jump block slightly as this will not cause any runtime
		 *      bugs (jump block is P0 load followed by jump).
		 */
		if (addr == default_init_addr)
			return true;
		addr = default_init_addr;
	}
	if (dxe_flags & DXE_BLOCK_FILL)
		flags |= LDR_FLAG_ZEROFILL;

	/* Punch a hole in the middle of this block if requested.
	 * This means the block we're punching just got split ... so if
	 * you're punching a hole in a block that by nature shouldn't be
	 * split, we'll just error out rather than trying to figure out
	 * how exactly to safely split it.  Also might be worth noting
	 * that while in master boot modes the address of the ignore
	 * block is irrelevant, in slave boot modes we need to actually
	 * read in the ignore data and put it somewhere, so we need to
	 * assume address 0 is suitable for this.
	 */
	if (opts->hole.offset) {
		size_t off = ftello(fp);
		uint32_t disk_count = (src ? count : 0);
		if (opts->hole.offset > off && opts->hole.offset < off + LDR_BLOCK_HEADER_LEN + disk_count) {
			uint32_t hole_count = opts->hole.length;

			if (dxe_flags & DXE_BLOCK_INIT)
				err("Punching holes in init blocks is not supported");

			/* fill up what we can to the punched location */
			ssize_t ssplit_count = opts->hole.offset - off - LDR_BLOCK_HEADER_LEN * 2;
			if (ssplit_count < LDR_BLOCK_HEADER_LEN) {
				/* leading hole is wicked small, so just expand the ignore block a bit */
				if (opts->hole.offset - off < LDR_BLOCK_HEADER_LEN)
					err("Unable to punch a hole soon enough");
				else
					hole_count += (opts->hole.offset - off - LDR_BLOCK_HEADER_LEN);
			} else if (src) {
				/* squeeze out a little of this block first */
				uint32_t split_count = ssplit_count;
				_bf53x_lfd_write_header(fp, flags, addr, split_count);
				if (fwrite(src, 1, split_count, fp) != split_count)
					return false;
				src += split_count;
				addr += split_count;
				count -= split_count;
			} else
				err("Punching holes with fill blocks?");

			/* finally write out hole */
			_bf53x_lfd_write_header(fp, flags | LDR_FLAG_IGNORE, 0, hole_count);
			if (opts->hole.filler_file) {
				FILE *filler_fp = fopen(opts->hole.filler_file, "rb");
				if (filler_fp) {
					size_t bytes, filled = 0;
					uint8_t filler_buf[8192];	/* random size */
					while (!feof(filler_fp)) {
						bytes = fread(filler_buf, 1, sizeof(filler_buf), filler_fp);
						filled += bytes;
						if (fwrite(filler_buf, 1, bytes, fp) != bytes)
							return false;
					}
					if (ferror(filler_fp))
						return false;
					if (filled > hole_count)
						err("filler file was bigger than the requested hole size");
					if (filled < hole_count)
						fseeko(fp, hole_count - filled, SEEK_CUR);
				} else
					return false;
			} else
				fseeko(fp, hole_count, SEEK_CUR);
		}
	}

	if (dxe_flags & DXE_BLOCK_FINAL)
		flags |= LDR_FLAG_FINAL;

	_bf53x_lfd_write_header(fp, flags, addr, count);

	if (src)
		return (fwrite(src, 1, count, fp) == count ? true : false);
	else
		return true;
}

uint32_t bf53x_lfd_dump_block(BLOCK *block, FILE *fp, bool dump_fill)
{
	BLOCK_HEADER *header = block->header;
	uint32_t wrote;

	if (!(header->flags & LDR_FLAG_ZEROFILL)) {
		wrote = fwrite(block->data, 1, header->byte_count, fp);
	} else if (dump_fill) {
		void *filler = xzalloc(header->byte_count);
		wrote = fwrite(filler, 1, header->byte_count, fp);
		free(filler);
	} else
		wrote = header->byte_count;

	if (wrote != header->byte_count)
		warnf("unable to write out");

	return header->target_address;
}


static const char * const bf537_aliases[] = { "BF534", "BF536", "BF537", NULL };
static const struct lfd_target bf537_lfd_target = {
	.name  = "BF537",
	.description = "Blackfin LDR handler for BF534/BF536/BF537",
	.aliases = bf537_aliases,
	.uart_boot = true,
	.iovec = {
		.read_block_header = bf53x_lfd_read_block_header,
		.display_dxe = bf53x_lfd_display_dxe,
		.write_ldr = bf53x_lfd_write_ldr,
		.write_block = bf53x_lfd_write_block,
		.dump_block = bf53x_lfd_dump_block,
	},
};

__attribute__((constructor))
static void bf537_lfd_target_register(void)
{
	lfd_target_register(&bf537_lfd_target);
}
