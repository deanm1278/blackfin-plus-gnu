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

#ifndef BLACKFIN_JTAG_H
#define BLACKFIN_JTAG_H

#include "jtag/jtag.h"
#include "target.h"
#include "blackfin_insn.h"

#define WPDA_DISABLE					0
#define WPDA_WRITE						1
#define WPDA_READ						2
#define WPDA_ALL						3

struct blackfin_hwwpt
{
	uint32_t addr;
	uint32_t len;
	int mode;
	/* If range is true, this hardware watchpoint is combined with the
	   next watchpoint to form a range watchpoint.  */
	bool range;
	/* True if this hardware watchpoint has been used, otherwise false.  */
	bool used;
};

#define BLACKFIN_MAX_HWBREAKPOINTS		6
#define BLACKFIN_MAX_HWWATCHPOINTS		2

struct blackfin_jtag
{
	struct target *target;

	uint16_t dbgctl;
    uint16_t dbgstat;
    uint64_t emuir_a;
    uint64_t emuir_b;
    uint64_t emudat_out;
    uint64_t emudat_in;

	uint32_t wpiactl;
	uint32_t wpdactl;
	uint32_t wpstat;

	uint32_t hwbps[BLACKFIN_MAX_HWBREAKPOINTS];
	struct blackfin_hwwpt hwwps[BLACKFIN_MAX_HWWATCHPOINTS];

	uint32_t emupc;
};

extern void blackfin_set_instr(struct blackfin_jtag *, uint8_t);

#define DECLARE_BLACKFIN_DBGCTL_BIT_SET(name)							\
	extern void blackfin_dbgctl_bit_set_##name(struct blackfin_jtag *);

#define DECLARE_BLACKFIN_DBGCTL_BIT_CLEAR(name)							\
	extern void blackfin_dbgctl_bit_clear_##name(struct blackfin_jtag *);

#define DECLARE_BLACKFIN_DBGCTL_BIT_IS(name)							\
	extern bool blackfin_dbgctl_bit_IS_##name(struct blackfin_jtag *);

#define DECLARE_BLACKFIN_DBGCTL_BIT_OP(name)							\
	DECLARE_BLACKFIN_DBGCTL_BIT_SET(name)								\
	DECLARE_BLACKFIN_DBGCTL_BIT_CLEAR(name)								\
	DECLARE_BLACKFIN_DBGCTL_BIT_IS(name)

DECLARE_BLACKFIN_DBGCTL_BIT_OP(sram_init)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(wakeup)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(sysrst)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(esstep)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emudatsz_32)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emudatsz_40)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emudatsz_48)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emuirlpsz_2)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emuirsz_64)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emuirsz_48)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emuirsz_32)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(empen)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emeen)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(emfen)
DECLARE_BLACKFIN_DBGCTL_BIT_OP(empwr)

#define DECLARE_BLACKFIN_DBGSTAT_BIT_IS(name)							\
	extern bool blackfin_dbgstat_is_##name(struct blackfin_jtag *);

DECLARE_BLACKFIN_DBGSTAT_BIT_IS(lpdec1)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(core_fault)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(idle)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(in_reset)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(lpdec0)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(bist_done)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emuack)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emuready)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emudiovf)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emudoovf)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emudif)
DECLARE_BLACKFIN_DBGSTAT_BIT_IS(emudof)

extern void blackfin_read_dbgstat(struct blackfin_jtag *);
extern void blackfin_wpstat_get(struct blackfin_jtag *);
extern void blackfin_wpstat_clear(struct blackfin_jtag *);
extern uint16_t blackfin_dbgstat_emucause(struct blackfin_jtag *);
extern void blackfin_emuir_set(struct blackfin_jtag *, uint32_t, tap_state_t);
extern void blackfin_emuir_set_2(struct blackfin_jtag *, uint32_t, uint32_t, tap_state_t);
extern void blackfin_emulation_enable(struct blackfin_jtag *);
extern void blackfin_emulation_disable(struct blackfin_jtag *);
extern void blackfin_emulation_trigger(struct blackfin_jtag *);
extern void blackfin_emulation_return(struct blackfin_jtag *);

extern void blackfin_register_set(struct blackfin_jtag *, enum core_regnum, uint32_t);
extern uint32_t blackfin_register_get(struct blackfin_jtag *, enum core_regnum);
extern uint32_t blackfin_get_p0(struct blackfin_jtag *);
extern uint32_t blackfin_get_r0(struct blackfin_jtag *);
extern void blackfin_set_p0(struct blackfin_jtag *, uint32_t);
extern void blackfin_set_r0(struct blackfin_jtag *, uint32_t);
extern void blackfin_check_emuready(struct blackfin_jtag *);
extern void blackfin_system_reset(struct blackfin_jtag *);
extern void blackfin_core_reset(struct blackfin_jtag *);
extern void blackfin_emupc_get(struct blackfin_jtag *);
extern void blackfin_emupc_reset(struct blackfin_jtag *);
extern uint32_t blackfin_emudat_get(struct blackfin_jtag *, tap_state_t);
extern void blackfin_emudat_set(struct blackfin_jtag *, uint32_t, tap_state_t);
extern void blackfin_wpu_init(struct blackfin_jtag *);
extern void blackfin_wpu_set_wpia(struct blackfin_jtag *, int, uint32_t, int);
extern void blackfin_wpu_set_wpda(struct blackfin_jtag *, int);
extern void blackfin_single_step(struct blackfin_jtag *, bool);
#endif /* BLACKFIN_JTAG_H */
