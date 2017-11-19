/***************************************************************************
 *   Copyright (C) 2011 by Analog Devices, Inc.                            *
 *   Written by Jie Zhang <jie.zhang@analog.com>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "blackfin_insn.h"

uint32_t
blackfin_gen_move (enum core_regnum dest, enum core_regnum src)
{
    uint32_t insn;

    insn = 0x3000;
    insn |= src & 0xf;
    insn |= (dest & 0xf) << 3;
    insn |= GROUP (src) << 6;
    insn |= GROUP (dest) << 9;

    return insn;
}

static uint32_t
gen_ldstidxi (enum core_regnum reg,
			  enum core_regnum ptr, int32_t offset, int w, int sz)
{
    uint32_t insn;

    insn = 0xe4000000;
    insn |= (reg & 0xf) << 16;
    insn |= (ptr & 0xf) << 19;

    switch (sz)
    {
        case 0:
            offset >>= 2;
            break;
        case 1:
            offset >>= 1;
            break;
        case 2:
            break;
        default:
            abort ();
    }
    if (offset > 32767 || offset < -32768)
        abort ();
    insn |= offset & 0xffff;

    insn |= w << 25;
    insn |= sz << 22;

    return insn;
}

uint32_t
blackfin_gen_load32_offset (enum core_regnum dest, enum core_regnum base, int32_t offset)
{
    return gen_ldstidxi (dest, base, offset, 0, 0);
}

uint32_t
blackfin_gen_store32_offset (enum core_regnum base, int32_t offset, enum core_regnum src)
{
    return gen_ldstidxi (src, base, offset, 1, 0);
}

uint32_t
blackfin_gen_load16z_offset (enum core_regnum dest, enum core_regnum base, int32_t offset)
{
    return gen_ldstidxi (dest, base, offset, 0, 1);
}

uint32_t
blackfin_gen_store16_offset (enum core_regnum base, int32_t offset, enum core_regnum src)
{
    return gen_ldstidxi (src, base, offset, 1, 1);
}

uint32_t
blackfin_gen_load8z_offset (enum core_regnum dest, enum core_regnum base, int32_t offset)
{
    return gen_ldstidxi (dest, base, offset, 0, 2);
}

uint32_t
blackfin_gen_store8_offset (enum core_regnum base, int32_t offset, enum core_regnum src)
{
    return gen_ldstidxi (src, base, offset, 1, 2);
}

static uint32_t
gen_ldst (enum core_regnum reg,
          enum core_regnum ptr, int post_dec, int w, int sz)
{
    uint32_t insn;

    insn = 0x9000;
    insn |= reg & 0xf;
    insn |= (ptr & 0xf) << 3;
    insn |= post_dec << 7;
    insn |= w << 9;
    insn |= sz << 10;

    return insn;
}

uint32_t
blackfin_gen_load32pi (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 0, 0, 0);
}

uint32_t
blackfin_gen_store32pi (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 0, 1, 0);
}

uint32_t
blackfin_gen_load16zpi (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 0, 0, 1);
}

uint32_t
blackfin_gen_store16pi (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 0, 1, 1);
}

uint32_t
blackfin_gen_load8zpi (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 0, 0, 2);
}

uint32_t
blackfin_gen_store8pi (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 0, 1, 2);
}

uint32_t
blackfin_gen_load32 (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 2, 0, 0);
}

uint32_t
blackfin_gen_store32 (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 2, 1, 0);
}

uint32_t
blackfin_gen_load16z (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 2, 0, 1);
}

uint32_t
blackfin_gen_store16 (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 2, 1, 1);
}

uint32_t
blackfin_gen_load8z (enum core_regnum dest, enum core_regnum base)
{
    return gen_ldst (dest, base, 2, 0, 2);
}

uint32_t
blackfin_gen_store8 (enum core_regnum base, enum core_regnum src)
{
    return gen_ldst (src, base, 2, 1, 2);
}

/* op
   0  prefetch
   1  flushinv
   2  flush
   3  iflush  */
static uint32_t
gen_flush_insn (enum core_regnum addr, int op, int post_modify)
{
    uint32_t insn;

    insn = 0x0240;
    insn |= addr & 0xf;
    insn |= op << 3;
    insn |= post_modify << 5;

    return insn;
}

uint32_t
blackfin_gen_iflush (enum core_regnum addr)
{
    return gen_flush_insn (addr, 3, 0);
}

uint32_t
blackfin_gen_iflush_pm (enum core_regnum addr)
{
    return gen_flush_insn (addr, 3, 1);
}

uint32_t
blackfin_gen_flush (enum core_regnum addr)
{
    return gen_flush_insn (addr, 2, 0);
}

uint32_t
blackfin_gen_flush_pm (enum core_regnum addr)
{
    return gen_flush_insn (addr, 2, 1);
}

uint32_t
blackfin_gen_flushinv (enum core_regnum addr)
{
    return gen_flush_insn (addr, 1, 0);
}

uint32_t
blackfin_gen_flushinv_pm (enum core_regnum addr)
{
    return gen_flush_insn (addr, 1, 1);
}

uint32_t
blackfin_gen_prefetch (enum core_regnum addr)
{
    return gen_flush_insn (addr, 0, 0);
}

uint32_t
blackfin_gen_prefetch_pm (enum core_regnum addr)
{
    return gen_flush_insn (addr, 0, 1);
}

uint32_t
blackfin_gen_jump_reg (enum core_regnum addr)
{
    uint32_t insn;

    insn = 0x0050;
    insn |= addr & 0x7;

    return insn;
}

uint32_t blackfin_gen_add_dreg_imm7 (enum core_regnum reg, int32_t imm7)
{
	uint32_t insn;

	insn = 0x6400;
	insn |= reg & 0x7;
	insn |= (imm7 & 0x7f) << 3;

	return insn;
}

uint32_t blackfin_gen_add_preg_imm7 (enum core_regnum reg, int32_t imm7)
{
	uint32_t insn;

	insn = 0x6c00;
	insn |= reg & 0x7;
	insn |= (imm7 & 0x7f) << 3;

	return insn;
}
