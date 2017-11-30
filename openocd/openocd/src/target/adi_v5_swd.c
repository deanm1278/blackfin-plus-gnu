/***************************************************************************
 *
 *   Copyright (C) 2010 by David Brownell
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ***************************************************************************/

/**
 * @file
 * Utilities to support ARM "Serial Wire Debug" (SWD), a low pin-count debug
 * link protocol used in cases where JTAG is not wanted.  This is coupled to
 * recent versions of ARM's "CoreSight" debug framework.  This specific code
 * is a transport level interface, with "target/arm_adi_v5.[hc]" code
 * understanding operation semantics, shared with the JTAG transport.
 *
 * Single-DAP support only.
 *
 * for details, see "ARM IHI 0031A"
 * ARM Debug Interface v5 Architecture Specification
 * especially section 5.3 for SWD protocol
 *
 * On many chips (most current Cortex-M3 parts) SWD is a run-time alternative
 * to JTAG.  Boards may support one or both.  There are also SWD-only chips,
 * (using SW-DP not SWJ-DP).
 *
 * Even boards that also support JTAG can benefit from SWD support, because
 * usually there's no way to access the SWO trace view mechanism in JTAG mode.
 * That is, trace access may require SWD support.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arm.h"
#include "arm.h"
#include "bfinplus.h"
#include "bfinplus_dap.h"
#include "arm_adi_v5.h"
#include <helper/time_support.h>

#include "target_type.h"

#include <transport/transport.h>
#include <jtag/interface.h>

#include <jtag/swd.h>

/* YUK! - but this is currently a global.... */
extern struct jtag_interface *jtag_interface;
static bool do_sync;

static void swd_finish_read(struct adiv5_dap *dap)
{
	const struct swd_driver *swd = jtag_interface->swd;
	if (dap->last_read != NULL) {
		swd->read_reg(dap, swd_cmd(true, false, DP_RDBUFF), dap->last_read);
		dap->last_read = NULL;
	}
}

static int swd_queue_dp_write(struct adiv5_dap *dap, unsigned reg,
		uint32_t data);
static int swd_queue_dp_read(struct adiv5_dap *dap, unsigned reg,
		uint32_t *data);

static void swd_clear_sticky_errors(struct adiv5_dap *dap)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	swd->write_reg(dap, swd_cmd(false,  false, DP_ABORT),
		STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
}

static int swd_run_inner(struct adiv5_dap *dap)
{
	const struct swd_driver *swd = jtag_interface->swd;
	int retval;

	retval = swd->run(dap);

	if (retval != ERROR_OK) {
		/* fault response */
		dap->do_reconnect = true;
	}

	return retval;
}

static int swd_connect(struct adiv5_dap *dap)
{
	uint32_t idcode;
	int status;

	/* FIXME validate transport config ... is the
	 * configured DAP present (check IDCODE)?
	 * Is *only* one DAP configured?
	 *
	 * MUST READ IDCODE
	 */

	/* Note, debugport_init() does setup too */
	jtag_interface->swd->switch_seq(dap, JTAG_TO_SWD);

	dap->do_reconnect = false;

	swd_queue_dp_read(dap, DP_IDCODE, &idcode);

	/* force clear all sticky faults */
	swd_clear_sticky_errors(dap);

	status = swd_run_inner(dap);

	if (status == ERROR_OK) {
		LOG_INFO("SWD IDCODE %#8.8" PRIx32, idcode);
		dap->do_reconnect = false;
	} else
		dap->do_reconnect = true;

	return status;
}

static inline int check_sync(struct adiv5_dap *dap)
{
	return do_sync ? swd_run_inner(dap) : ERROR_OK;
}

static int swd_check_reconnect(struct adiv5_dap *dap)
{
	if (dap->do_reconnect)
		return swd_connect(dap);

	return ERROR_OK;
}

static int swd_queue_ap_abort(struct adiv5_dap *dap, uint8_t *ack)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	swd->write_reg(dap, swd_cmd(false,  false, DP_ABORT),
		DAPABORT | STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
	return check_sync(dap);
}

/** Select the DP register bank matching bits 7:4 of reg. */
static void swd_queue_dp_bankselect(struct adiv5_dap *dap, unsigned reg)
{
	uint32_t select_dp_bank = (reg & 0x000000F0) >> 4;

	if (reg == DP_SELECT)
		return;

	if (select_dp_bank == dap->dp_bank_value)
		return;

	dap->dp_bank_value = select_dp_bank;
	select_dp_bank |= dap->ap_current | dap->ap_bank_value;

	swd_queue_dp_write(dap, DP_SELECT, select_dp_bank);
}

static int swd_queue_dp_read(struct adiv5_dap *dap, unsigned reg,
		uint32_t *data)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	int retval = swd_check_reconnect(dap);
	if (retval != ERROR_OK)
		return retval;

	swd_queue_dp_bankselect(dap, reg);
	swd->read_reg(dap, swd_cmd(true,  false, reg), data);

	return check_sync(dap);
}

static int swd_queue_dp_write(struct adiv5_dap *dap, unsigned reg,
		uint32_t data)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	int retval = swd_check_reconnect(dap);
	if (retval != ERROR_OK)
		return retval;

	swd_finish_read(dap);
	swd_queue_dp_bankselect(dap, reg);
	swd->write_reg(dap, swd_cmd(false,  false, reg), data);

	return check_sync(dap);
}

/** Select the AP register bank matching bits 7:4 of reg. */
static void swd_queue_ap_bankselect(struct adiv5_dap *dap, unsigned reg)
{
	uint32_t select_ap_bank = reg & 0x000000F0;

	if (select_ap_bank == dap->ap_bank_value)
		return;

	dap->ap_bank_value = select_ap_bank;
	select_ap_bank |= dap->ap_current | dap->dp_bank_value;

	swd_queue_dp_write(dap, DP_SELECT, select_ap_bank);
}

static int swd_queue_ap_read(struct adiv5_dap *dap, unsigned reg,
		uint32_t *data)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	int retval = swd_check_reconnect(dap);
	if (retval != ERROR_OK)
		return retval;

	swd_queue_ap_bankselect(dap, reg);
	swd->read_reg(dap, swd_cmd(true,  true, reg), dap->last_read);
	dap->last_read = data;

	return check_sync(dap);
}

static int swd_queue_ap_write(struct adiv5_dap *dap, unsigned reg,
		uint32_t data)
{
	const struct swd_driver *swd = jtag_interface->swd;
	assert(swd);

	int retval = swd_check_reconnect(dap);
	if (retval != ERROR_OK)
		return retval;

	swd_finish_read(dap);
	swd_queue_ap_bankselect(dap, reg);
	swd->write_reg(dap, swd_cmd(false,  true, reg), data);

	return check_sync(dap);
}

/** Executes all queued DAP operations. */
static int swd_run(struct adiv5_dap *dap)
{
	swd_finish_read(dap);
	return swd_run_inner(dap);
}

const struct dap_ops swd_dap_ops = {
	.is_swd = true,

	.queue_dp_read = swd_queue_dp_read,
	.queue_dp_write = swd_queue_dp_write,
	.queue_ap_read = swd_queue_ap_read,
	.queue_ap_write = swd_queue_ap_write,
	.queue_ap_abort = swd_queue_ap_abort,
	.run = swd_run,
};

/*
 * This represents the bits which must be sent out on TMS/SWDIO to
 * switch a DAP implemented using an SWJ-DP module into SWD mode.
 * These bits are stored (and transmitted) LSB-first.
 *
 * See the DAP-Lite specification, section 2.2.5 for information
 * about making the debug link select SWD or JTAG.  (Similar info
 * is in a few other ARM documents.)
 */
static const uint8_t jtag2swd_bitseq[] = {
	/* More than 50 TCK/SWCLK cycles with TMS/SWDIO high,
	 * putting both JTAG and SWD logic into reset state.
	 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* Switching sequence enables SWD and disables JTAG
	 * NOTE: bits in the DP's IDCODE may expose the need for
	 * an old/obsolete/deprecated sequence (0xb6 0xed).
	 */
	0x9e, 0xe7,
	/* More than 50 TCK/SWCLK cycles with TMS/SWDIO high,
	 * putting both JTAG and SWD logic into reset state.
	 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

/**
 * Put the debug link into SWD mode, if the target supports it.
 * The link's initial mode may be either JTAG (for example,
 * with SWJ-DP after reset) or SWD.
 *
 * @param target Enters SWD mode (if possible).
 *
 * Note that targets using the JTAG-DP do not support SWD, and that
 * some targets which could otherwise support it may have have been
 * configured to disable SWD signaling
 *
 * @return ERROR_OK or else a fault code.
 */
int dap_to_swd(struct target *target)
{
	int retval;
	struct target_type *type = target->type;
	struct adiv5_dap *dap;
	//TODO: we need a better way to do this
	if(!strcmp(type->name, "bfinplus")){
			struct bfinplus_common *bfin = target_to_bfinplus(target);
			struct bfinplus_dap *bfin_dap = &bfin->dap;
			dap = &bfin_dap->dap;
	}
	else{
		struct arm *arm = target_to_arm(target);
		dap = arm->dap;
	}

	if (!dap) {
		LOG_ERROR("SWD mode is not available");
		return ERROR_FAIL;
	}

	LOG_DEBUG("Enter SWD mode");

	/* REVISIT it's ugly to need to make calls to a "jtag"
	 * subsystem if the link may not be in JTAG mode...
	 */

	retval =  jtag_add_tms_seq(8 * sizeof(jtag2swd_bitseq),
			jtag2swd_bitseq, TAP_INVALID);
	if (retval == ERROR_OK)
		retval = jtag_execute_queue();

	/* set up the DAP's ops vector for SWD mode. */
	dap->ops = &swd_dap_ops;

	return retval;
}

COMMAND_HANDLER(handle_swd_wcr)
{
	int retval;
	uint32_t wcr;
	unsigned trn, scale = 0;
	struct target *target = get_current_target(CMD_CTX);
	struct target_type *type = target->type;
	struct adiv5_dap *dap;
	//TODO: we need a better way to do this
	if(!strcmp(type->name, "bfinplus")){
			struct bfinplus_common *bfin = target_to_bfinplus(target);
			struct bfinplus_dap *bfin_dap = &bfin->dap;
			dap = &bfin_dap->dap;
	}
	else{
		struct arm *arm = target_to_arm(target);
		dap = arm->dap;
	}

	switch (CMD_ARGC) {
	/* no-args: just dump state */
	case 0:
		/*retval = swd_queue_dp_read(dap, DP_WCR, &wcr); */
		retval = dap_queue_dp_read(dap, DP_WCR, &wcr);
		if (retval == ERROR_OK)
			dap->ops->run(dap);
		if (retval != ERROR_OK) {
			LOG_ERROR("can't read WCR?");
			return retval;
		}

		command_print(CMD_CTX,
			"turnaround=%" PRIu32 ", prescale=%" PRIu32,
			WCR_TO_TRN(wcr),
			WCR_TO_PRESCALE(wcr));
	return ERROR_OK;

	case 2:		/* TRN and prescale */
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[1], scale);
		if (scale > 7) {
			LOG_ERROR("prescale %d is too big", scale);
			return ERROR_FAIL;
		}
		/* FALL THROUGH */

	case 1:		/* TRN only */
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[0], trn);
		if (trn < 1 || trn > 4) {
			LOG_ERROR("turnaround %d is invalid", trn);
			return ERROR_FAIL;
		}

		wcr = ((trn - 1) << 8) | scale;
		/* FIXME
		 * write WCR ...
		 * then, re-init adapter with new TRN
		 */
		LOG_ERROR("can't yet modify WCR");
		return ERROR_FAIL;

	default:	/* too many arguments */
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
}

static const struct command_registration swd_commands[] = {
	{
		/*
		 * Set up SWD and JTAG targets identically, unless/until
		 * infrastructure improves ...  meanwhile, ignore all
		 * JTAG-specific stuff like IR length for SWD.
		 *
		 * REVISIT can we verify "just one SWD DAP" here/early?
		 */
		.name = "newdap",
		.jim_handler = jim_jtag_newtap,
		.mode = COMMAND_CONFIG,
		.help = "declare a new SWD DAP"
	},
	{
		.name = "wcr",
		.handler = handle_swd_wcr,
		.mode = COMMAND_ANY,
		.help = "display or update DAP's WCR register",
		.usage = "turnaround (1..4), prescale (0..7)",
	},

	/* REVISIT -- add a command for SWV trace on/off */
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration swd_handlers[] = {
	{
		.name = "swd",
		.mode = COMMAND_ANY,
		.help = "SWD command group",
		.chain = swd_commands,
	},
	COMMAND_REGISTRATION_DONE
};

static int swd_select(struct command_context *ctx)
{
	int retval;

	retval = register_commands(ctx, NULL, swd_handlers);

	if (retval != ERROR_OK)
		return retval;

	const struct swd_driver *swd = jtag_interface->swd;

	 /* be sure driver is in SWD mode; start
	  * with hardware default TRN (1), it can be changed later
	  */
	if (!swd || !swd->read_reg || !swd->write_reg || !swd->init) {
		LOG_DEBUG("no SWD driver?");
		return ERROR_FAIL;
	}

	retval = swd->init();
	if (retval != ERROR_OK) {
		LOG_DEBUG("can't init SWD driver");
		return retval;
	}

	/* force DAP into SWD mode (not JTAG) */
	/*retval = dap_to_swd(target);*/

	if (ctx->current_target) {
		/* force DAP into SWD mode (not JTAG) */
		struct target *target = get_current_target(ctx);
		retval = dap_to_swd(target);
	}

	return retval;
}

static int swd_init(struct command_context *ctx)
{
	struct target *target = get_current_target(ctx);
	struct target_type *type = target->type;
	struct adiv5_dap *dap;
	//TODO: we need a better way to do this
	if(!strcmp(type->name, "bfinplus")){
		struct bfinplus_common *bfin = target_to_bfinplus(target);
		struct bfinplus_dap *bfin_dap = &bfin->dap;
		dap = &bfin_dap->dap;
	
		/* Force the DAP's ops vector for SWD mode.
		 * messy - is there a better way? */
		dap->ops = &swd_dap_ops;
	}
	else{
		struct arm *arm = target_to_arm(target);
		dap = arm->dap;
	
		/* Force the DAP's ops vector for SWD mode.
		 * messy - is there a better way? */
		arm->dap->ops = &swd_dap_ops;
	}

	return swd_connect(dap);
}

static struct transport swd_transport = {
	.name = "swd",
	.select = swd_select,
	.init = swd_init,
};

static void swd_constructor(void) __attribute__((constructor));
static void swd_constructor(void)
{
	transport_register(&swd_transport);
}

/** Returns true if the current debug session
 * is using SWD as its transport.
 */
bool transport_is_swd(void)
{
	return get_current_transport() == &swd_transport;
}
