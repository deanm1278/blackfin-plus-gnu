/*
 * File: dxe_jump.h
 *
 * Copyright 2006-2007 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Binary code for doing a "jump <address>"
 */

#ifndef __DXE_JUMP__
#define __DXE_JUMP__

#include "blackfin_defines.h"

/* for debugging purposes:
 * $ bfin-uclinux-gcc -x assembler-with-cpp -c dxe_jump.h -o dxe_jump.o
 * $ bfin-uclinux-objdump -d dxe_jump.o
 * $ bfin-uclinux-objcopy -I elf32-bfin -O binary dxe_jump.o dxe_jump.bin
 *
 * $ bfin-uclinux-gcc -x assembler-with-cpp -S -o - dxe_jump.h
 */
# ifdef __ASSEMBLER__

	P0.L = LO(ADDRESS);
	P0.H = HI(ADDRESS);
	jump (P0);

# else

#define DXE_JUMP_CODE_SIZE 12
static inline uint8_t *dxe_jump_code(const uint32_t address)
{
	static uint8_t jump_buf[DXE_JUMP_CODE_SIZE] = {
		[0] 0x08, [1] 0xE1, [2] 0xFF, [3] 0xFF, /* P0.L = LO(address); */
		[4] 0x48, [5] 0xE1, [6] 0xFF, [7] 0xFF, /* P0.H = HI(address); */
		[8] 0x50, [9] 0x00,                     /* jump (P0); */
		    0x00,     0x00                      /* nop; (pad to align to 32bits) */
	};

	FILL_ADDR_32(jump_buf, address, 2, 3, 6, 7);

	return jump_buf;
}

# endif

#endif
