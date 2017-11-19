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

#ifndef BLACKFIN_INSN_H
#define BLACKFIN_INSN_H

/* High-Nibble: group code, low nibble: register code.  */
#define T_REG_R                         0x00
#define T_REG_P                         0x10
#define T_REG_I                         0x20
#define T_REG_B                         0x30
#define T_REG_L                         0x34
#define T_REG_M                         0x24
#define T_REG_A                         0x40

enum core_regnum
{
    REG_R0 = T_REG_R, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
    REG_P0 = T_REG_P, REG_P1, REG_P2, REG_P3, REG_P4, REG_P5, REG_SP, REG_FP,
    REG_I0 = T_REG_I, REG_I1, REG_I2, REG_I3,
    REG_M0 = T_REG_M, REG_M1, REG_M2, REG_M3,
    REG_B0 = T_REG_B, REG_B1, REG_B2, REG_B3,
    REG_L0 = T_REG_L, REG_L1, REG_L2, REG_L3,
    REG_A0x = T_REG_A, REG_A0w, REG_A1x, REG_A1w,
    REG_ASTAT = 0x46,
    REG_RETS = 0x47,
    REG_LC0 = 0x60, REG_LT0, REG_LB0, REG_LC1, REG_LT1, REG_LB1,
    REG_CYCLES, REG_CYCLES2,
    REG_USP = 0x70, REG_SEQSTAT, REG_SYSCFG,
    REG_RETI, REG_RETX, REG_RETN, REG_RETE, REG_EMUDAT,
};

#define CLASS_MASK                      0xf0
#define GROUP(x)                        (((x) & CLASS_MASK) >> 4)
#define DREG_P(x)                       (((x) & CLASS_MASK) == T_REG_R)
#define PREG_P(x)                       (((x) & CLASS_MASK) == T_REG_P)

#define BLACKFIN_INSN_NOP				0x0000
#define BLACKFIN_INSN_RTE				0x0014
#define BLACKFIN_INSN_CSYNC				0x0023
#define BLACKFIN_INSN_SSYNC				0x0024
#define BLACKFIN_INSN_ILLEGAL			0xffffffff

uint32_t blackfin_gen_move (enum core_regnum dest, enum core_regnum src);
uint32_t blackfin_gen_load32_offset (enum core_regnum dest, enum core_regnum base, int32_t offset);
uint32_t blackfin_gen_store32_offset (enum core_regnum base, int32_t offset, enum core_regnum src);
uint32_t blackfin_gen_load16z_offset (enum core_regnum dest, enum core_regnum base, int32_t offset);
uint32_t blackfin_gen_store16_offset (enum core_regnum base, int32_t offset, enum core_regnum src);
uint32_t blackfin_gen_load8z_offset (enum core_regnum dest, enum core_regnum base, int32_t offset);
uint32_t blackfin_gen_store8_offset (enum core_regnum base, int32_t offset, enum core_regnum src);
uint32_t blackfin_gen_load32pi (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store32pi (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_load16zpi (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store16pi (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_load8zpi (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store8pi (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_load32 (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store32 (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_load16z (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store16 (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_load8z (enum core_regnum dest, enum core_regnum base);
uint32_t blackfin_gen_store8 (enum core_regnum base, enum core_regnum src);
uint32_t blackfin_gen_iflush (enum core_regnum addr);
uint32_t blackfin_gen_iflush_pm (enum core_regnum addr);
uint32_t blackfin_gen_flush (enum core_regnum addr);
uint32_t blackfin_gen_flush_pm (enum core_regnum addr);
uint32_t blackfin_gen_flushinv (enum core_regnum addr);
uint32_t blackfin_gen_flushinv_pm (enum core_regnum addr);
uint32_t blackfin_gen_prefetch (enum core_regnum addr);
uint32_t blackfin_gen_prefetch_pm (enum core_regnum addr);
uint32_t blackfin_gen_jump_reg (enum core_regnum addr);

uint32_t blackfin_gen_add_dreg_imm7 (enum core_regnum reg, int32_t imm7);
uint32_t blackfin_gen_add_preg_imm7 (enum core_regnum reg, int32_t imm7);

#endif /* BLACKFIN_INSN_H */
