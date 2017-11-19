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

#include <assert.h>
#include <time.h>

#include "helper/binarybuffer.h"
#include "jtag/jtag.h"

#include "blackfin.h"
#include "blackfin_jtag.h"
#include "blackfin_mem.h"

/* tap instructions */
#define BLACKFIN_IDCODE			0x02
#define BLACKFIN_DBGCTL			0x04
#define BLACKFIN_EMUIR			0x08
#define BLACKFIN_DBGSTAT		0x0c
#define BLACKFIN_EMUDAT			0x14
#define BLACKFIN_EMUPC			0x1e
#define BLACKFIN_BYPASS			0x1f

static struct {
	uint16_t dbgctl_sram_init;
	uint16_t dbgctl_wakeup;
	uint16_t dbgctl_sysrst;
	uint16_t dbgctl_esstep;
	uint16_t dbgctl_emudatsz_32;
	uint16_t dbgctl_emudatsz_40;
	uint16_t dbgctl_emudatsz_48;
	uint16_t dbgctl_emudatsz_mask;
	uint16_t dbgctl_emuirlpsz_2;
	uint16_t dbgctl_emuirsz_64;
	uint16_t dbgctl_emuirsz_48;
	uint16_t dbgctl_emuirsz_32;
	uint16_t dbgctl_emuirsz_mask;
	uint16_t dbgctl_empen;
	uint16_t dbgctl_emeen;
	uint16_t dbgctl_emfen;
	uint16_t dbgctl_empwr;

	uint16_t dbgstat_lpdec1;
	uint16_t dbgstat_core_fault;
	uint16_t dbgstat_idle;
	uint16_t dbgstat_in_reset;
	uint16_t dbgstat_lpdec0;
	uint16_t dbgstat_bist_done;
	uint16_t dbgstat_emucause_mask;
	uint16_t dbgstat_emuack;
	uint16_t dbgstat_emuready;
	uint16_t dbgstat_emudiovf;
	uint16_t dbgstat_emudoovf;
	uint16_t dbgstat_emudif;
	uint16_t dbgstat_emudof;
} bits = {
	0x1000, /* DBGCTL_SRAM_INIT */
	0x0800, /* DBGCTL_WAKEUP */
	0x0400, /* DBGCTL_SYSRST */
	0x0200, /* DBGCTL_ESSTEP */
	0x0000, /* DBGCTL_EMUDATSZ_32 */
	0x0080, /* DBGCTL_EMUDATSZ_40 */
	0x0100, /* DBGCTL_EMUDATSZ_48 */
	0x0180, /* DBGCTL_EMUDATSZ_MASK */
	0x0040, /* DBGCTL_EMUIRLPSZ_2 */
	0x0000, /* DBGCTL_EMUIRSZ_64 */
	0x0010, /* DBGCTL_EMUIRSZ_48 */
	0x0020, /* DBGCTL_EMUIRSZ_32 */
	0x0030, /* DBGCTL_EMUIRSZ_MASK */
	0x0008, /* DBGCTL_EMPEN */
	0x0004, /* DBGCTL_EMEEN */
	0x0002, /* DBGCTL_EMFEN */
	0x0001, /* DBGCTL_EMPWR */

	0x8000, /* DBGSTAT_LPDEC1 */
	0x4000, /* DBGSTAT_CORE_FAULT */
	0x2000, /* DBGSTAT_IDLE */
	0x1000, /* DBGSTAT_IN_RESET */
	0x0800, /* DBGSTAT_LPDEC0 */
	0x0400, /* DBGSTAT_BIST_DONE */
	0x03c0, /* DBGSTAT_EMUCAUSE_MASK */
	0x0020, /* DBGSTAT_EMUACK */
	0x0010, /* DBGSTAT_EMUREADY */
	0x0008, /* DBGSTAT_EMUDIOVF */
	0x0004, /* DBGSTAT_EMUDOOVF */
	0x0002, /* DBGSTAT_EMUDIF */
	0x0001, /* DBGSTAT_EMUDOF */
};

#define SWRST							0xffc00100

#define WPIACTL							0xffe07000
#define WPIACTL_WPAND					0x02000000
#define WPIACTL_EMUSW5					0x01000000
#define WPIACTL_EMUSW4					0x00800000
#define WPIACTL_WPICNTEN5				0x00400000
#define WPIACTL_WPICNTEN4				0x00200000
#define WPIACTL_WPIAEN5					0x00100000
#define WPIACTL_WPIAEN4					0x00080000
#define WPIACTL_WPIRINV45				0x00040000
#define WPIACTL_WPIREN45				0x00020000
#define WPIACTL_EMUSW3					0x00010000
#define WPIACTL_EMUSW2					0x00008000
#define WPIACTL_WPICNTEN3				0x00004000
#define WPIACTL_WPICNTEN2				0x00002000
#define WPIACTL_WPIAEN3					0x00001000
#define WPIACTL_WPIAEN2					0x00000800
#define WPIACTL_WPIRINV23				0x00000400
#define WPIACTL_WPIREN23				0x00000200
#define WPIACTL_EMUSW1					0x00000100
#define WPIACTL_EMUSW0					0x00000080
#define WPIACTL_WPICNTEN1				0x00000040
#define WPIACTL_WPICNTEN0				0x00000020
#define WPIACTL_WPIAEN1					0x00000010
#define WPIACTL_WPIAEN0					0x00000008
#define WPIACTL_WPIRINV01				0x00000004
#define WPIACTL_WPIREN01				0x00000002
#define WPIACTL_WPPWR					0x00000001
#define WPIA0							0xffe07040

#define WPDACTL							0xffe07100
#define WPDACTL_WPDACC1_R				0x00002000
#define WPDACTL_WPDACC1_W				0x00001000
#define WPDACTL_WPDACC1_A				0x00003000
#define WPDACTL_WPDSRC1_1				0x00000800
#define WPDACTL_WPDSRC1_0				0x00000400
#define WPDACTL_WPDSRC1_A				0x00000c00
#define WPDACTL_WPDACC0_R				0x00000200
#define WPDACTL_WPDACC0_W				0x00000100
#define WPDACTL_WPDACC0_A				0x00000300
#define WPDACTL_WPDSRC0_1				0x00000080
#define WPDACTL_WPDSRC0_0				0x00000040
#define WPDACTL_WPDSRC0_A				0x000000c0
#define WPDACTL_WPDCNTEN1				0x00000020
#define WPDACTL_WPDCNTEN0				0x00000010
#define WPDACTL_WPDAEN1					0x00000008
#define WPDACTL_WPDAEN0					0x00000004
#define WPDACTL_WPDRINV01				0x00000002
#define WPDACTL_WPDREN01				0x00000001
#define WPDA0							0xffe07140

#define WPSTAT							0xffe07200

static void buf_set(void *_buffer, unsigned first, unsigned num, uint32_t value)
{
	buf_set_u32(_buffer, first, num, flip_u32(value, num));
}

static uint32_t buf_get(const void *_buffer, unsigned first, unsigned num)
{
	return flip_u32(buf_get_u32(_buffer, first, num), num);
}

/* Force moving from Pause-DR to Shift-DR to go through Capture-DR.
   Otherwise, it will go Exit2-DR, Shift-DR. Similar for IR scan.

   The code is copied from arm11_dbgtap.c. We should move it to generic
   code. */

static const tap_state_t blackfin_move_pi_to_si_via_ci[] =
{
	TAP_IREXIT2, TAP_IRUPDATE, TAP_DRSELECT, TAP_IRSELECT, TAP_IRCAPTURE, TAP_IRSHIFT
};

static void blackfin_add_ir_scan_vc(struct jtag_tap *tap, struct scan_field *fields,
		tap_state_t state)
{
	if (cmd_queue_cur_state == TAP_IRPAUSE)
		jtag_add_pathmove(ARRAY_SIZE(blackfin_move_pi_to_si_via_ci), blackfin_move_pi_to_si_via_ci);

	jtag_add_ir_scan(tap, fields, state);
}

static const tap_state_t blackfin_move_pd_to_sd_via_cd[] =
{
	TAP_DREXIT2, TAP_DRUPDATE, TAP_DRSELECT, TAP_DRCAPTURE, TAP_DRSHIFT
};

void blackfin_add_dr_scan_vc(struct jtag_tap *tap, int num_fields, struct scan_field *fields,
		tap_state_t state)
{
	if (cmd_queue_cur_state == TAP_DRPAUSE)
		jtag_add_pathmove(ARRAY_SIZE(blackfin_move_pd_to_sd_via_cd), blackfin_move_pd_to_sd_via_cd);

	jtag_add_dr_scan(tap, num_fields, fields, state);
}

static void blackfin_add_wait_clocks(void)
{
	jtag_add_clocks(blackfin_wait_clocks);
}

void blackfin_set_instr(struct blackfin_jtag *jtag_info, uint8_t new_instr)
{
	struct jtag_tap *tap;

	tap = jtag_info->target->tap;
	assert(tap != NULL);

	if (buf_get_u32(tap->cur_instr, 0, tap->ir_length) != new_instr)
	{
		struct scan_field field;
		uint8_t t[4];

		field.num_bits = tap->ir_length;
		field.out_value = t;
		buf_set_u32(t, 0, field.num_bits, new_instr);
		field.in_value = NULL;

		blackfin_add_ir_scan_vc(tap, &field, TAP_IRPAUSE);
	}
}

#define BLACKFIN_DBGCTL_BIT_SET(name)									\
	void blackfin_dbgctl_bit_set_##name(struct blackfin_jtag *jtag_info) \
	{																	\
		jtag_info->dbgctl |= bits.dbgctl_##name;						\
	}

#define BLACKFIN_DBGCTL_BIT_CLEAR(name)									\
	void blackfin_dbgctl_bit_clear_##name(struct blackfin_jtag *jtag_info) \
	{																	\
		jtag_info->dbgctl &= ~bits.dbgctl_##name;						\
	}

#define BLACKFIN_DBGCTL_IS(name)										\
	bool blackfin_dbgctl_is_##name(struct blackfin_jtag *jtag_info)		\
	{																	\
		if (jtag_info->dbgctl & bits.dbgctl_##name)						\
			return true;												\
		else															\
			return false;												\
	}

#define BLACKFIN_DBGCTL_BIT_OP(name)									\
	BLACKFIN_DBGCTL_BIT_SET(name)										\
	BLACKFIN_DBGCTL_BIT_CLEAR(name)										\
	BLACKFIN_DBGCTL_IS(name)

BLACKFIN_DBGCTL_BIT_OP(sram_init)
BLACKFIN_DBGCTL_BIT_OP(wakeup)
BLACKFIN_DBGCTL_BIT_OP(sysrst)
BLACKFIN_DBGCTL_BIT_OP(esstep)
BLACKFIN_DBGCTL_BIT_OP(emudatsz_32)
BLACKFIN_DBGCTL_BIT_OP(emudatsz_40)
BLACKFIN_DBGCTL_BIT_OP(emudatsz_48)
BLACKFIN_DBGCTL_BIT_OP(emuirlpsz_2)
BLACKFIN_DBGCTL_BIT_OP(emuirsz_64)
BLACKFIN_DBGCTL_BIT_OP(emuirsz_48)
BLACKFIN_DBGCTL_BIT_OP(emuirsz_32)
BLACKFIN_DBGCTL_BIT_OP(empen)
BLACKFIN_DBGCTL_BIT_OP(emeen)
BLACKFIN_DBGCTL_BIT_OP(emfen)
BLACKFIN_DBGCTL_BIT_OP(empwr)

#define BLACKFIN_DBGSTAT_BIT_IS(name)									\
	bool blackfin_dbgstat_is_##name(struct blackfin_jtag *jtag_info)	\
	{																	\
		if (jtag_info->dbgstat & bits.dbgstat_##name)					\
			return true;												\
		else															\
			return false;												\
	}

BLACKFIN_DBGSTAT_BIT_IS(lpdec1)
BLACKFIN_DBGSTAT_BIT_IS(core_fault)
BLACKFIN_DBGSTAT_BIT_IS(idle)
BLACKFIN_DBGSTAT_BIT_IS(in_reset)
BLACKFIN_DBGSTAT_BIT_IS(lpdec0)
BLACKFIN_DBGSTAT_BIT_IS(bist_done)
BLACKFIN_DBGSTAT_BIT_IS(emuack)
BLACKFIN_DBGSTAT_BIT_IS(emuready)
BLACKFIN_DBGSTAT_BIT_IS(emudiovf)
BLACKFIN_DBGSTAT_BIT_IS(emudoovf)
BLACKFIN_DBGSTAT_BIT_IS(emudif)
BLACKFIN_DBGSTAT_BIT_IS(emudof)

static void blackfin_shift_dbgctl(struct blackfin_jtag *jtag_info, int state)
{
	struct scan_field field;
	uint8_t t[4];

	LOG_DEBUG("shift DBGCTL = %04x end state is %s", (unsigned) jtag_info->dbgctl, tap_state_name(state));

	field.num_bits = 16;
	field.out_value = t;
	buf_set(t, 0, field.num_bits, jtag_info->dbgctl);
	field.in_value = NULL;
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, state);

	if (state == TAP_IDLE)
		blackfin_add_wait_clocks();
}

static void blackfin_emuir_setup_field(struct scan_field *field, uint8_t *t, uint32_t insn)
{
	if ((insn & 0xffff0000) == 0)
		insn <<= 16;

	field->num_bits = 32;
	field->out_value = t;
	buf_set(t, 0, field->num_bits, insn);
	field->in_value = NULL;
}

void blackfin_emuir_set(struct blackfin_jtag *jtag_info, uint32_t insn, int state)
{
	struct scan_field field;
	uint8_t t[4];

	/* If the EMUIRLPSZ_2 is set in DBGCTL, clear it. */
	if (blackfin_dbgctl_is_emuirlpsz_2(jtag_info))
	{
		blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
		blackfin_dbgctl_bit_clear_emuirlpsz_2(jtag_info);
		blackfin_shift_dbgctl(jtag_info, TAP_DRPAUSE);
	}

	blackfin_set_instr(jtag_info, BLACKFIN_EMUIR);

	blackfin_emuir_setup_field(&field, t, insn);
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, state);
	jtag_info->emuir_a = insn;

	if (state == TAP_IDLE)
		blackfin_add_wait_clocks();
}

void blackfin_emuir_set_2(struct blackfin_jtag *jtag_info, uint32_t insn1, uint32_t insn2, int state)
{
	struct scan_field field1, field2;
	uint8_t t[4];

	/* If the EMUIRLPSZ_2 is clear in DBGCTL, set it. */
	if (!blackfin_dbgctl_is_emuirlpsz_2(jtag_info))
	{
		blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
		blackfin_dbgctl_bit_set_emuirlpsz_2(jtag_info);
		blackfin_shift_dbgctl(jtag_info, TAP_DRPAUSE);
	}

	blackfin_set_instr(jtag_info, BLACKFIN_EMUIR);

	blackfin_emuir_setup_field(&field2, t, insn2);
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field2, TAP_DRPAUSE);
	jtag_info->emuir_b = insn2;

	blackfin_emuir_setup_field(&field1, t, insn1);
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field1, state);
	jtag_info->emuir_a = insn1;

	if (state == TAP_IDLE)
		blackfin_add_wait_clocks();
}

/*
static void blackfin_emuir_set_nop(struct blackfin_jtag *jtag_info)
{
	if (blackfin_dbgctl_is_emuirlpsz_2(jtag_info))
		blackfin_emuir_set_2(jtag_info, BLACKFIN_INSN_NOP, BLACKFIN_INSN_NOP, TAP_DRPAUSE);
	else
		blackfin_emuir_set(jtag_info, BLACKFIN_INSN_NOP, TAP_DRPAUSE);
}
*/

void blackfin_read_dbgstat(struct blackfin_jtag *jtag_info)
{
	struct scan_field field;
	uint16_t dbgstat;
	uint8_t t[4];
	int retval;

	blackfin_set_instr(jtag_info, BLACKFIN_DBGSTAT);

	field.num_bits = 16;
	field.out_value = NULL;
	field.in_value = t;

	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, TAP_DRPAUSE);

	retval = jtag_execute_queue();
	assert(retval == ERROR_OK);

	dbgstat = buf_get(t, 0, 16);

	if (jtag_info->dbgstat != dbgstat)
		LOG_DEBUG("DBGSTAT = %04x (OLD %04x)",
				(unsigned) dbgstat,
				(unsigned) jtag_info->dbgstat);

	jtag_info->dbgstat = dbgstat;
}

uint16_t blackfin_dbgstat_emucause(struct blackfin_jtag *jtag_info)
{
	uint16_t mask, emucause;

	mask = bits.dbgstat_emucause_mask;
	emucause = jtag_info->dbgstat & mask;

	while (!(mask & 0x1))
	{
		mask >>= 1;
		emucause >>= 1;
	}

	return emucause;
}

void blackfin_wpstat_get(struct blackfin_jtag *jtag_info)
{
	uint32_t p0, r0;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, WPSTAT);
	blackfin_emuir_set(jtag_info, blackfin_gen_load32_offset(REG_R0, REG_P0, 0), TAP_IDLE);
	jtag_info->wpstat = blackfin_register_get(jtag_info, REG_R0);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

void blackfin_wpstat_clear(struct blackfin_jtag *jtag_info)
{
	uint32_t p0, r0;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, WPSTAT);
	blackfin_set_r0(jtag_info, jtag_info->wpstat);
	blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, 0, REG_R0), TAP_IDLE);
	jtag_info->wpstat = 0;

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

void blackfin_emulation_enable(struct blackfin_jtag *jtag_info)
{
	/* TODO check if emulation has already been enabled. */

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);

	blackfin_dbgctl_bit_set_empwr(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);

	blackfin_dbgctl_bit_set_emfen(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);

	blackfin_dbgctl_bit_set_emuirsz_32(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);
}

void blackfin_emulation_disable(struct blackfin_jtag *jtag_info)
{
	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_clear_empwr(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);
}

void blackfin_emulation_trigger(struct blackfin_jtag *jtag_info)
{
	/* TODO check if we have already been in emulation mode. */

	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_NOP, TAP_DRPAUSE);

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_set_wakeup(jtag_info);
	blackfin_dbgctl_bit_set_emeen(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);
}

void blackfin_emulation_return(struct blackfin_jtag *jtag_info)
{
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_RTE, TAP_DRPAUSE);

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_clear_emeen(jtag_info);
	blackfin_dbgctl_bit_clear_wakeup(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);
}

uint32_t blackfin_register_get(struct blackfin_jtag *jtag_info, enum core_regnum reg)
{
	struct scan_field field;
	uint8_t t[4];
	uint32_t r0 = 0;
	int retval;

	if (DREG_P(reg) || PREG_P(reg))
	{
		blackfin_emuir_set(jtag_info, blackfin_gen_move(REG_EMUDAT, reg), TAP_IDLE);
	}
	else
	{
		r0 = blackfin_get_r0(jtag_info);
		blackfin_emuir_set_2(jtag_info, blackfin_gen_move(REG_R0, reg), blackfin_gen_move(REG_EMUDAT, REG_R0), TAP_IDLE);
	}

	blackfin_set_instr(jtag_info, BLACKFIN_EMUDAT);

	field.num_bits = 32;
	field.out_value = NULL;
	field.in_value = t;
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, TAP_DRPAUSE);

	retval = jtag_execute_queue();
	assert(retval == ERROR_OK);

	if (!DREG_P(reg) && !PREG_P(reg))
		blackfin_set_r0(jtag_info, r0);

	return buf_get(t, 0, 32);
}

void blackfin_register_set(struct blackfin_jtag *jtag_info, enum core_regnum reg, uint32_t value)
{
	struct scan_field field;
	uint8_t t[4];
	uint32_t r0 = 0;

	if (!DREG_P(reg) && !PREG_P(reg))
		r0 = blackfin_get_r0(jtag_info);

	blackfin_set_instr(jtag_info, BLACKFIN_EMUDAT);

	field.num_bits = 32;
	field.out_value = t;
	buf_set(t, 0, field.num_bits, value);
	field.in_value = NULL;
	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, TAP_DRPAUSE);

	if (DREG_P(reg) || PREG_P(reg))
	{
		blackfin_emuir_set(jtag_info, blackfin_gen_move(reg, REG_EMUDAT), TAP_IDLE);
	}
	else
	{
		blackfin_emuir_set_2(jtag_info, blackfin_gen_move(REG_R0, REG_EMUDAT), blackfin_gen_move(reg, REG_R0), TAP_IDLE);
		blackfin_set_r0(jtag_info, r0);
	}
}

uint32_t blackfin_get_p0(struct blackfin_jtag *jtag_info)
{
	return blackfin_register_get(jtag_info, REG_P0);
}

uint32_t blackfin_get_r0(struct blackfin_jtag *jtag_info)
{
	return blackfin_register_get(jtag_info, REG_R0);
}

void blackfin_set_p0(struct blackfin_jtag *jtag_info, uint32_t value)
{
	blackfin_register_set(jtag_info, REG_P0, value);
}

void blackfin_set_r0(struct blackfin_jtag *jtag_info, uint32_t value)
{
	blackfin_register_set(jtag_info, REG_R0, value);
}

void blackfin_check_emuready(struct blackfin_jtag *jtag_info)
{
	int emuready;

	blackfin_read_dbgstat(jtag_info);
	if (blackfin_dbgstat_is_emuready(jtag_info))
		emuready = 1;
	else
		emuready = 0;

	assert(emuready);
}

/* system reset by writing to SWRST MMR */
void blackfin_system_reset(struct blackfin_jtag *jtag_info)
{
	uint32_t p0, r0;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	/*
	 * Flush all system events like cache line fills.  Otherwise,
	 * when we reset the system side, any events that the core was
	 * waiting on no longer exist, and the core hangs.
	 */
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_SSYNC, TAP_IDLE);

	/* Write 0x7 to SWRST to start system reset. */
	blackfin_set_p0(jtag_info, SWRST);
	blackfin_set_r0(jtag_info, 0x7);
	blackfin_emuir_set(jtag_info, blackfin_gen_store16_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	/*
	 * Delay at least 10 SCLKs instead of doing an SSYNC insn.
	 * Since the system is being reset, the sync signal might
	 * not be asserted, and so the core hangs waiting for it.
	 * The magic "10" number was given to us by ADI designers
	 * who looked at the schematic and ran some simulations.
	 */
	usleep(100);

	/* Write 0x0 to SWRST to stop system reset. */
	blackfin_set_r0(jtag_info, 0);
	blackfin_emuir_set(jtag_info, blackfin_gen_store16_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	/* Delay at least 1 SCLK; see comment above for more info. */
	usleep(100);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

static void blackfin_wait_in_reset(struct blackfin_jtag *jtag_info)
{
	int in_reset;
	int waited = 0;
	const struct timespec reset_wait = {0, 5000000};

 try_again:

	blackfin_read_dbgstat(jtag_info);
	if (blackfin_dbgstat_is_in_reset(jtag_info))
		in_reset = 1;
	else
		in_reset = 0;

	if (waited)
		assert(in_reset);

	if (!in_reset)
	{
		nanosleep (&reset_wait, NULL);
		waited = 1;
		goto try_again;
	}
}

static void blackfin_wait_reset(struct blackfin_jtag *jtag_info)
{
	int in_reset;
	int waited = 0;
	const struct timespec reset_wait = {0, 5000000};

 try_again:

	blackfin_read_dbgstat(jtag_info);
	if (blackfin_dbgstat_is_in_reset(jtag_info))
		in_reset = 1;
	else
		in_reset = 0;

	if (waited)
		assert(!in_reset);

	if (in_reset)
	{
		nanosleep (&reset_wait, NULL);
		waited = 1;
		goto try_again;
	}
}

/* core reset by setting SYSRST bit in DBGCTL */
void blackfin_core_reset(struct blackfin_jtag *jtag_info)
{
	blackfin_emulation_disable(jtag_info);

	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_NOP, TAP_DRPAUSE);

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_set_sram_init(jtag_info);
	blackfin_dbgctl_bit_set_sysrst(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);

	blackfin_wait_in_reset(jtag_info);

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_clear_sysrst(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);

	blackfin_wait_reset(jtag_info);

	blackfin_emulation_enable(jtag_info);
	blackfin_emulation_trigger(jtag_info);

	blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
	blackfin_dbgctl_bit_clear_sram_init(jtag_info);
	blackfin_shift_dbgctl(jtag_info, TAP_IDLE);
}

void blackfin_emupc_get(struct blackfin_jtag *jtag_info)
{
	struct scan_field field;
	uint8_t t[4];
	uint32_t value;
	int retval;

	blackfin_set_instr(jtag_info, BLACKFIN_EMUPC);

	field.num_bits = 32;
	field.out_value = NULL;
	field.in_value = t;

	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, TAP_DRPAUSE);

	retval = jtag_execute_queue();
	assert(retval == ERROR_OK);

	value = buf_get(t, 0, field.num_bits);
	jtag_info->emupc = value;
}

void blackfin_emupc_reset(struct blackfin_jtag *jtag_info)
{
	struct blackfin_common *blackfin = target_to_blackfin(jtag_info->target);
	uint32_t p0;

	p0 = blackfin_get_p0(jtag_info);
	blackfin_set_p0(jtag_info, blackfin->l1_map->l1_code);
	blackfin_emuir_set(jtag_info, blackfin_gen_jump_reg(REG_P0), TAP_IDLE);
	blackfin_set_p0(jtag_info, p0);
}

void blackfin_emudat_set(struct blackfin_jtag *jtag_info, uint32_t value, tap_state_t state)
{
	struct scan_field field;
	uint8_t t[4];

	blackfin_set_instr(jtag_info, BLACKFIN_EMUDAT);

	field.num_bits = 32;
	field.out_value = t;
	buf_set(t, 0, field.num_bits, value);
	field.in_value = NULL;

	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, state);
	jtag_info->emudat_in = value;

	if (state == TAP_IDLE)
		blackfin_add_wait_clocks();
}

uint32_t blackfin_emudat_get(struct blackfin_jtag *jtag_info, tap_state_t state)
{
	struct scan_field field;
	uint8_t t[4];
	uint32_t value;
	const uint8_t seq = 0;
	int retval;

	if (state == TAP_IDLE)
	{
		if (cmd_queue_cur_state != TAP_RESET && cmd_queue_cur_state != TAP_IDLE)
			jtag_add_statemove(TAP_IDLE);
		else
			jtag_add_tms_seq(1, &seq, TAP_IDLE);

		blackfin_add_wait_clocks();
	}

	blackfin_set_instr(jtag_info, BLACKFIN_EMUDAT);

	field.num_bits = 32;
	field.out_value = NULL;
	field.in_value = t;

	blackfin_add_dr_scan_vc(jtag_info->target->tap, 1, &field, TAP_DRPAUSE);

	retval = jtag_execute_queue();
	assert(retval == ERROR_OK);

	value = buf_get(t, 0, field.num_bits);
	jtag_info->emudat_out = value;

	return value;
}

void blackfin_wpu_init(struct blackfin_jtag *jtag_info)
{
	uint32_t p0, r0;
	uint32_t wpiactl, wpdactl;

	LOG_DEBUG("-");

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_set_p0(jtag_info, WPIACTL);

	wpiactl = WPIACTL_WPPWR;

	blackfin_set_r0(jtag_info, wpiactl);

	blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	wpiactl |= WPIACTL_EMUSW5 | WPIACTL_EMUSW4 | WPIACTL_EMUSW3;
	wpiactl |= WPIACTL_EMUSW2 | WPIACTL_EMUSW1 | WPIACTL_EMUSW0;

	blackfin_set_r0(jtag_info, wpiactl);
	blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, 0, REG_R0), TAP_IDLE);

	wpdactl = WPDACTL_WPDSRC1_A | WPDACTL_WPDSRC0_A;

	blackfin_set_r0(jtag_info, wpdactl);
	blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, WPDACTL - WPIACTL, REG_R0),
					   TAP_IDLE);

	blackfin_set_r0(jtag_info, 0);
	blackfin_emuir_set(jtag_info, blackfin_gen_store32_offset(REG_P0, WPSTAT - WPIACTL, REG_R0),
					   TAP_IDLE);

	jtag_info->wpiactl = wpiactl;
	jtag_info->wpdactl = wpdactl;

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

static uint32_t wpiaen[] = {
	WPIACTL_WPIAEN0,
	WPIACTL_WPIAEN1,
	WPIACTL_WPIAEN2,
	WPIACTL_WPIAEN3,
	WPIACTL_WPIAEN4,
	WPIACTL_WPIAEN5,
};

void blackfin_wpu_set_wpia(struct blackfin_jtag *jtag_info, int n, uint32_t addr, int enable)
{
	uint32_t p0, r0;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_register_set(jtag_info, REG_P0, WPIACTL);
	if (enable)
	{
		jtag_info->wpiactl += wpiaen[n];
		blackfin_register_set(jtag_info, REG_R0, addr);
		blackfin_emuir_set(jtag_info,
			blackfin_gen_store32_offset(REG_P0, WPIA0 + 4 * n - WPIACTL, REG_R0),
			TAP_IDLE);
	}
	else
	{
		jtag_info->wpiactl &= ~wpiaen[n];
	}

	blackfin_register_set(jtag_info, REG_R0, jtag_info->wpiactl);
	blackfin_emuir_set(jtag_info,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0),
		TAP_IDLE);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

void blackfin_wpu_set_wpda(struct blackfin_jtag *jtag_info, int n)
{
	uint32_t p0, r0;
	uint32_t addr = jtag_info->hwwps[n].addr;
	uint32_t len = jtag_info->hwwps[n].len;
	int mode = jtag_info->hwwps[n].mode;
	bool range = jtag_info->hwwps[n].range;

	p0 = blackfin_get_p0(jtag_info);
	r0 = blackfin_get_r0(jtag_info);

	blackfin_register_set(jtag_info, REG_P0, WPDACTL);

	/* Currently LEN > 4 iff RANGE.  But in future, it can change.  */
	if (len > 4 || range)
	{
		assert (n == 0);

		switch (mode)
		{
		case WPDA_DISABLE:
			jtag_info->wpdactl &= ~WPDACTL_WPDREN01;
			break;
        case WPDA_WRITE:
			jtag_info->wpdactl &= ~WPDACTL_WPDACC0_R;
			jtag_info->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_W;
			break;
        case WPDA_READ:
			jtag_info->wpdactl &= ~WPDACTL_WPDACC0_W;
			jtag_info->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_R;
			break;
        case WPDA_ALL:
			jtag_info->wpdactl |= WPDACTL_WPDREN01 | WPDACTL_WPDACC0_A;
			break;
        default:
			abort ();
        }
		
		if (mode != WPDA_DISABLE)
        {
			blackfin_register_set(jtag_info, REG_R0, addr - 1);
			blackfin_emuir_set(jtag_info,
				blackfin_gen_store32_offset(REG_P0, WPDA0 - WPDACTL, REG_R0),
				TAP_IDLE);
			blackfin_register_set(jtag_info, REG_R0, addr + len - 1);
			blackfin_emuir_set(jtag_info,
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 - WPDACTL, REG_R0),
				TAP_IDLE);
        }
    }
	else
	{
		if (n == 0)
			switch (mode)
			{
			case WPDA_DISABLE:
				jtag_info->wpdactl &= ~WPDACTL_WPDAEN0;
				break;
			case WPDA_WRITE:
				jtag_info->wpdactl &= ~WPDACTL_WPDACC0_R;
				jtag_info->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_W;
				break;
			case WPDA_READ:
				jtag_info->wpdactl &= ~WPDACTL_WPDACC0_W;
				jtag_info->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_R;
				break;
			case WPDA_ALL:
				jtag_info->wpdactl |= WPDACTL_WPDAEN0 | WPDACTL_WPDACC0_A;
				break;
			default:
				abort ();
			}
		else
			switch (mode)
			{
			case WPDA_DISABLE:
				jtag_info->wpdactl &= ~WPDACTL_WPDAEN1;
				break;
			case WPDA_WRITE:
				jtag_info->wpdactl &= ~WPDACTL_WPDACC1_R;
				jtag_info->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_W;
				break;
			case WPDA_READ:
				jtag_info->wpdactl &= ~WPDACTL_WPDACC1_W;
				jtag_info->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_R;
				break;
			case WPDA_ALL:
				jtag_info->wpdactl |= WPDACTL_WPDAEN1 | WPDACTL_WPDACC1_A;
				break;
			default:
				abort ();
			}
		if (mode != WPDA_DISABLE)
        {
			blackfin_register_set(jtag_info, REG_R0, addr);
			blackfin_emuir_set(jtag_info,
				blackfin_gen_store32_offset(REG_P0, WPDA0 + 4 * n - WPDACTL, REG_R0),
				TAP_IDLE);
        }
    }

	blackfin_register_set(jtag_info, REG_R0, jtag_info->wpdactl);
	blackfin_emuir_set(jtag_info,
		blackfin_gen_store32_offset(REG_P0, 0, REG_R0),
		TAP_IDLE);

	blackfin_set_p0(jtag_info, p0);
	blackfin_set_r0(jtag_info, r0);
}

void blackfin_single_step(struct blackfin_jtag *jtag_info, bool in_stepping)
{
	if (!in_stepping)
	{
		blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
		blackfin_dbgctl_bit_set_esstep(jtag_info);
		blackfin_shift_dbgctl(jtag_info, TAP_DRPAUSE);
	}
	blackfin_emuir_set(jtag_info, BLACKFIN_INSN_RTE, TAP_IDLE);
	blackfin_check_emuready(jtag_info);
	if (!in_stepping)
	{
		blackfin_set_instr(jtag_info, BLACKFIN_DBGCTL);
		blackfin_dbgctl_bit_clear_esstep(jtag_info);
		blackfin_shift_dbgctl(jtag_info, TAP_DRPAUSE);
	}
}
