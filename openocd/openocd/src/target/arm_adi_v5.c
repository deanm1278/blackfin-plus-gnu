/***************************************************************************
 *   Copyright (C) 2006 by Magnus Lundin                                   *
 *   lundin@mlu.mine.nu                                                    *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2009-2010 by Oyvind Harboe                              *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2009-2010 by David Brownell                             *
 *                                                                         *
 *   Copyright (C) 2013 by Andreas Fritiofson                              *
 *   andreas.fritiofson@gmail.com                                          *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

/**
 * @file
 * This file implements support for the ARM Debug Interface version 5 (ADIv5)
 * debugging architecture.  Compared with previous versions, this includes
 * a low pin-count Serial Wire Debug (SWD) alternative to JTAG for message
 * transport, and focusses on memory mapped resources as defined by the
 * CoreSight architecture.
 *
 * A key concept in ADIv5 is the Debug Access Port, or DAP.  A DAP has two
 * basic components:  a Debug Port (DP) transporting messages to and from a
 * debugger, and an Access Port (AP) accessing resources.  Three types of DP
 * are defined.  One uses only JTAG for communication, and is called JTAG-DP.
 * One uses only SWD for communication, and is called SW-DP.  The third can
 * use either SWD or JTAG, and is called SWJ-DP.  The most common type of AP
 * is used to access memory mapped resources and is called a MEM-AP.  Also a
 * JTAG-AP is also defined, bridging to JTAG resources; those are uncommon.
 *
 * This programming interface allows DAP pipelined operations through a
 * transaction queue.  This primarily affects AP operations (such as using
 * a MEM-AP to access memory or registers).  If the current transaction has
 * not finished by the time the next one must begin, and the ORUNDETECT bit
 * is set in the DP_CTRL_STAT register, the SSTICKYORUN status is set and
 * further AP operations will fail.  There are two basic methods to avoid
 * such overrun errors.  One involves polling for status instead of using
 * transaction piplining.  The other involves adding delays to ensure the
 * AP has enough time to complete one operation before starting the next
 * one.  (For JTAG these delays are controlled by memaccess_tck.)
 */

/*
 * Relevant specifications from ARM include:
 *
 * ARM(tm) Debug Interface v5 Architecture Specification    ARM IHI 0031A
 * CoreSight(tm) v1.0 Architecture Specification            ARM IHI 0029B
 *
 * CoreSight(tm) DAP-Lite TRM, ARM DDI 0316D
 * Cortex-M3(tm) TRM, ARM DDI 0337G
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag/interface.h"
#include "arm.h"
#include "arm_adi_v5.h"
#include <jtag/swd.h>
#include <helper/time_support.h>

#if 0
/* ARM ADI Specification requires at least 10 bits used for TAR autoincrement  */

/*
	uint32_t tar_block_size(uint32_t address)
	Return the largest block starting at address that does not cross a tar block size alignment boundary
*/
static uint32_t max_tar_block_size(uint32_t tar_autoincr_block, uint32_t address)
{
	return tar_autoincr_block - ((tar_autoincr_block - 1) & address);
}
#endif

/***************************************************************************
 *                                                                         *
 * DP and MEM-AP  register access  through APACC and DPACC                 *
 *                                                                         *
***************************************************************************/

/**
 * Select one of the APs connected to the specified DAP.  The
 * selection is implicitly used with future AP transactions.
 * This is a NOP if the specified AP is already selected.
 *
 * @param dap The DAP
 * @param apsel Number of the AP to (implicitly) use with further
 *	transactions.  This normally identifies a MEM-AP.
 */
void dap_ap_select(struct adiv5_dap *dap, uint8_t ap)
{
	uint32_t new_ap = (ap << 24) & 0xFF000000;

	if (new_ap != dap->ap_current) {
		dap->ap_current = new_ap;
		/* Switching AP invalidates cached values.
		 * Values MUST BE UPDATED BEFORE AP ACCESS.
		 */
		dap->ap_bank_value = -1;
		dap->ap_csw_value = -1;
		dap->ap_tar_value = -1;
	}
}

static int dap_setup_accessport_csw(struct adiv5_dap *dap, uint32_t csw)
{
#if 0
	csw = csw | CSW_DBGSWENABLE | CSW_MASTER_DEBUG | CSW_HPROT |
		dap->apcsw[dap->ap_current >> 24];
#else
	csw = csw | CSW_HPROT | CSW_SPIDEN | (1 << 24) |
	CSW_DEVICE_EN | dap->apcsw[dap->ap_current >> 24];
#endif

	if (csw != dap->ap_csw_value) {
		/* LOG_DEBUG("DAP: Set CSW %x",csw); */
		int retval = dap_queue_ap_write(dap, AP_REG_CSW, csw);
		if (retval != ERROR_OK)
			return retval;
		dap->ap_csw_value = csw;
	}
	return ERROR_OK;
}

static int dap_setup_accessport_tar(struct adiv5_dap *dap, uint32_t tar)
{
	if (tar != dap->ap_tar_value || dap->ap_csw_value & CSW_ADDRINC_MASK) {
		/* LOG_DEBUG("DAP: Set TAR %x",tar); */
		int retval = dap_queue_ap_write(dap, AP_REG_TAR, tar);
		if (retval != ERROR_OK)
			return retval;
		dap->ap_tar_value = tar;
	}
	return ERROR_OK;
}

/**
 * Queue transactions setting up transfer parameters for the
 * currently selected MEM-AP.
 *
 * Subsequent transfers using registers like AP_REG_DRW or AP_REG_BD2
 * initiate data reads or writes using memory or peripheral addresses.
 * If the CSW is configured for it, the TAR may be automatically
 * incremented after each transfer.
 *
 * @todo Rename to reflect it being specifically a MEM-AP function.
 *
 * @param dap The DAP connected to the MEM-AP.
 * @param csw MEM-AP Control/Status Word (CSW) register to assign.  If this
 *	matches the cached value, the register is not changed.
 * @param tar MEM-AP Transfer Address Register (TAR) to assign.  If this
 *	matches the cached address, the register is not changed.
 *
 * @return ERROR_OK if the transaction was properly queued, else a fault code.
 */
int dap_setup_accessport(struct adiv5_dap *dap, uint32_t csw, uint32_t tar)
{
	int retval;
	retval = dap_setup_accessport_csw(dap, csw);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_setup_accessport_tar(dap, tar);
	if (retval != ERROR_OK)
		return retval;
	return ERROR_OK;
}

/**
 * Asynchronous (queued) read of a word from memory or a system register.
 *
 * @param dap The DAP connected to the MEM-AP performing the read.
 * @param address Address of the 32-bit word to read; it must be
 *	readable by the currently selected MEM-AP.
 * @param value points to where the word will be stored when the
 *	transaction queue is flushed (assuming no errors).
 *
 * @return ERROR_OK for success.  Otherwise a fault code.
 */
int mem_ap_read_u32(struct adiv5_dap *dap, uint32_t address,
		uint32_t *value)
{
	int retval;

	/* Use banked addressing (REG_BDx) to avoid some link traffic
	 * (updating TAR) when reading several consecutive addresses.
	 */
	retval = dap_setup_accessport(dap, CSW_32BIT | CSW_ADDRINC_OFF,
			address & 0xFFFFFFF0);
	if (retval != ERROR_OK)
		return retval;

	return dap_queue_ap_read(dap, AP_REG_BD0 | (address & 0xC), value);
}

/**
 * Synchronous read of a word from memory or a system register.
 * As a side effect, this flushes any queued transactions.
 *
 * @param dap The DAP connected to the MEM-AP performing the read.
 * @param address Address of the 32-bit word to read; it must be
 *	readable by the currently selected MEM-AP.
 * @param value points to where the result will be stored.
 *
 * @return ERROR_OK for success; *value holds the result.
 * Otherwise a fault code.
 */
int mem_ap_read_atomic_u32(struct adiv5_dap *dap, uint32_t address,
		uint32_t *value)
{
	int retval;

	retval = mem_ap_read_u32(dap, address, value);
	if (retval != ERROR_OK)
		return retval;

	return dap_run(dap);
}

/**
 * Asynchronous (queued) write of a word to memory or a system register.
 *
 * @param dap The DAP connected to the MEM-AP.
 * @param address Address to be written; it must be writable by
 *	the currently selected MEM-AP.
 * @param value Word that will be written to the address when transaction
 *	queue is flushed (assuming no errors).
 *
 * @return ERROR_OK for success.  Otherwise a fault code.
 */
int mem_ap_write_u32(struct adiv5_dap *dap, uint32_t address,
		uint32_t value)
{
	int retval;

	/* Use banked addressing (REG_BDx) to avoid some link traffic
	 * (updating TAR) when writing several consecutive addresses.
	 */
	retval = dap_setup_accessport(dap, CSW_32BIT | CSW_ADDRINC_OFF,
			address & 0xFFFFFFF0);
	if (retval != ERROR_OK)
		return retval;

	return dap_queue_ap_write(dap, AP_REG_BD0 | (address & 0xC),
			value);
}

/**
 * Synchronous write of a word to memory or a system register.
 * As a side effect, this flushes any queued transactions.
 *
 * @param dap The DAP connected to the MEM-AP.
 * @param address Address to be written; it must be writable by
 *	the currently selected MEM-AP.
 * @param value Word that will be written.
 *
 * @return ERROR_OK for success; the data was written.  Otherwise a fault code.
 */
int mem_ap_write_atomic_u32(struct adiv5_dap *dap, uint32_t address,
		uint32_t value)
{
	int retval = mem_ap_write_u32(dap, address, value);

	if (retval != ERROR_OK)
		return retval;

	return dap_run(dap);
}

/**
 * Synchronous write of a block of memory, using a specific access size.
 *
 * @param dap The DAP connected to the MEM-AP.
 * @param buffer The data buffer to write. No particular alignment is assumed.
 * @param size Which access size to use, in bytes. 1, 2 or 4.
 * @param count The number of writes to do (in size units, not bytes).
 * @param address Address to be written; it must be writable by the currently selected MEM-AP.
 * @param addrinc Whether the target address should be increased for each write or not. This
 *  should normally be true, except when writing to e.g. a FIFO.
 * @return ERROR_OK on success, otherwise an error code.
 */
int mem_ap_write(struct adiv5_dap *dap, const uint8_t *buffer, uint32_t size, uint32_t count,
		uint32_t address, bool addrinc)
{
	size_t nbytes = size * count;
	const uint32_t csw_addrincr = addrinc ? CSW_ADDRINC_SINGLE : CSW_ADDRINC_OFF;
	uint32_t csw_size;
	uint32_t addr_xor;
	int retval;

	/* TI BE-32 Quirks mode:
	 * Writes on big-endian TMS570 behave very strangely. Observed behavior:
	 *   size   write address   bytes written in order
	 *   4      TAR ^ 0         (val >> 24), (val >> 16), (val >> 8), (val)
	 *   2      TAR ^ 2         (val >> 8), (val)
	 *   1      TAR ^ 3         (val)
	 * For example, if you attempt to write a single byte to address 0, the processor
	 * will actually write a byte to address 3.
	 *
	 * To make writes of size < 4 work as expected, we xor a value with the address before
	 * setting the TAP, and we set the TAP after every transfer rather then relying on
	 * address increment. */

	if (size == 4) {
		csw_size = CSW_32BIT;
		addr_xor = 0;
	} else if (size == 2) {
		csw_size = CSW_16BIT;
		addr_xor = dap->ti_be_32_quirks ? 2 : 0;
	} else if (size == 1) {
		csw_size = CSW_8BIT;
		addr_xor = dap->ti_be_32_quirks ? 3 : 0;
	} else {
		return ERROR_TARGET_UNALIGNED_ACCESS;
	}

	if (dap->unaligned_access_bad && (address % size != 0))
		return ERROR_TARGET_UNALIGNED_ACCESS;

	retval = dap_setup_accessport_tar(dap, address ^ addr_xor);
	if (retval != ERROR_OK)
		return retval;

	while (nbytes > 0) {
		uint32_t this_size = size;
#if 0
		/* Select packed transfer if possible */
		if (addrinc && dap->packed_transfers && nbytes >= 4
				&& max_tar_block_size(dap->tar_autoincr_block, address) >= 4) {
			this_size = 4;
			retval = dap_setup_accessport_csw(dap, csw_size | CSW_ADDRINC_PACKED);
		} else {
			retval = dap_setup_accessport_csw(dap, csw_size | csw_addrincr);
		}
#else
		retval = dap_setup_accessport_csw(dap, csw_size | csw_addrincr);
#endif
		if (retval != ERROR_OK)
			break;

		/* How many source bytes each transfer will consume, and their location in the DRW,
		 * depends on the type of transfer and alignment. See ARM document IHI0031C. */
		uint32_t outvalue = 0;
		if (dap->ti_be_32_quirks) {
			switch (this_size) {
			case 4:
				outvalue |= (uint32_t)*buffer++ << 8 * (3 ^ (address++ & 3) ^ addr_xor);
				outvalue |= (uint32_t)*buffer++ << 8 * (3 ^ (address++ & 3) ^ addr_xor);
				outvalue |= (uint32_t)*buffer++ << 8 * (3 ^ (address++ & 3) ^ addr_xor);
				outvalue |= (uint32_t)*buffer++ << 8 * (3 ^ (address++ & 3) ^ addr_xor);
				break;
			case 2:
				outvalue |= (uint32_t)*buffer++ << 8 * (1 ^ (address++ & 3) ^ addr_xor);
				outvalue |= (uint32_t)*buffer++ << 8 * (1 ^ (address++ & 3) ^ addr_xor);
				break;
			case 1:
				outvalue |= (uint32_t)*buffer++ << 8 * (0 ^ (address++ & 3) ^ addr_xor);
				break;
			}
		} else {
			switch (this_size) {
			case 4:
				outvalue |= (uint32_t)*buffer++ << 8 * (address++ & 3);
				outvalue |= (uint32_t)*buffer++ << 8 * (address++ & 3);
			case 2:
				outvalue |= (uint32_t)*buffer++ << 8 * (address++ & 3);
			case 1:
				outvalue |= (uint32_t)*buffer++ << 8 * (address++ & 3);
			}
		}

		nbytes -= this_size;

		retval = dap_queue_ap_write(dap, AP_REG_DRW, outvalue);
		if (retval != ERROR_OK)
			break;

		/* Rewrite TAR if it wrapped or we're xoring addresses */
		if (addrinc && (addr_xor || (address % dap->tar_autoincr_block < size && nbytes > 0))) {
			retval = dap_setup_accessport_tar(dap, address ^ addr_xor);
			if (retval != ERROR_OK)
				break;
		}
	}

	/* REVISIT: Might want to have a queued version of this function that does not run. */
	if (retval == ERROR_OK)
		retval = dap_run(dap);

	if (retval != ERROR_OK)
		return retval;

	if (addrinc) {
		uint32_t tar;
		retval = dap_queue_ap_read(dap, AP_REG_TAR, &tar);
		if (retval == ERROR_OK)
			retval = dap_run(dap);
		if (retval != ERROR_OK)
			return retval;
		/* Update TAR to reflect incremented address */
		dap->ap_tar_value = tar;
#if 0
		if (tar != address)
			LOG_ERROR("TAR auto-increment does not work");
#endif
	}

	if (retval != ERROR_OK) {
		uint32_t tar;
		if (dap_queue_ap_read(dap, AP_REG_TAR, &tar) == ERROR_OK
				&& dap_run(dap) == ERROR_OK)
			LOG_ERROR("Failed to write memory at 0x%08"PRIx32, tar);
		else
			LOG_ERROR("Failed to write memory and, additionally, failed to find out where");
	}

	return retval;
}

/**
 * Synchronous read of a block of memory, using a specific access size.
 *
 * @param dap The DAP connected to the MEM-AP.
 * @param buffer The data buffer to receive the data. No particular alignment is assumed.
 * @param size Which access size to use, in bytes. 1, 2 or 4.
 * @param count The number of reads to do (in size units, not bytes).
 * @param address Address to be read; it must be readable by the currently selected MEM-AP.
 * @param addrinc Whether the target address should be increased after each read or not. This
 *  should normally be true, except when reading from e.g. a FIFO.
 * @return ERROR_OK on success, otherwise an error code.
 */
int mem_ap_read(struct adiv5_dap *dap, uint8_t *buffer, uint32_t size, uint32_t count,
		uint32_t adr, bool addrinc)
{
	size_t nbytes = size * count;
	const uint32_t csw_addrincr = addrinc ? CSW_ADDRINC_SINGLE : CSW_ADDRINC_OFF;
	uint32_t csw_size;
	uint32_t address = adr;
	int retval;

	/* TI BE-32 Quirks mode:
	 * Reads on big-endian TMS570 behave strangely differently than writes.
	 * They read from the physical address requested, but with DRW byte-reversed.
	 * For example, a byte read from address 0 will place the result in the high bytes of DRW.
	 * Also, packed 8-bit and 16-bit transfers seem to sometimes return garbage in some bytes,
	 * so avoid them. */

	if (size == 4)
		csw_size = CSW_32BIT;
	else if (size == 2)
		csw_size = CSW_16BIT;
	else if (size == 1)
		csw_size = CSW_8BIT;
	else
		return ERROR_TARGET_UNALIGNED_ACCESS;

	if (dap->unaligned_access_bad && (adr % size != 0))
		return ERROR_TARGET_UNALIGNED_ACCESS;

	/* Allocate buffer to hold the sequence of DRW reads that will be made. This is a significant
	 * over-allocation if packed transfers are going to be used, but determining the real need at
	 * this point would be messy. */
	uint32_t *read_buf = malloc(count * sizeof(uint32_t));
	uint32_t *read_ptr = read_buf;
	if (read_buf == NULL) {
		LOG_ERROR("Failed to allocate read buffer");
		return ERROR_FAIL;
	}

	retval = dap_setup_accessport_tar(dap, address);
	if (retval != ERROR_OK) {
		free(read_buf);
		return retval;
	}

	/* Queue up all reads. Each read will store the entire DRW word in the read buffer. How many
	 * useful bytes it contains, and their location in the word, depends on the type of transfer
	 * and alignment. */
	while (nbytes > 0) {
		uint32_t this_size = size;
#if 0
		/* Select packed transfer if possible */
		if (addrinc && dap->packed_transfers && nbytes >= 4
				&& max_tar_block_size(dap->tar_autoincr_block, address) >= 4) {
			this_size = 4;
			retval = dap_setup_accessport_csw(dap, csw_size | CSW_ADDRINC_PACKED);
		} else {
			retval = dap_setup_accessport_csw(dap, csw_size | csw_addrincr);
		}
#else
		retval = dap_setup_accessport_csw(dap, csw_size | csw_addrincr);
#endif
		if (retval != ERROR_OK)
			break;

		retval = dap_queue_ap_read(dap, AP_REG_DRW, read_ptr++);
		if (retval != ERROR_OK)
			break;

		nbytes -= this_size;
		address += this_size;

		/* Rewrite TAR if it wrapped */
		if (addrinc && address % dap->tar_autoincr_block < size && nbytes > 0) {
			retval = dap_setup_accessport_tar(dap, address);
			if (retval != ERROR_OK)
				break;
		}
	}

	if (retval == ERROR_OK)
		retval = dap_run(dap);

	if (retval != ERROR_OK)
		return retval;

	if (addrinc) {
		uint32_t tar;
		retval = dap_queue_ap_read(dap, AP_REG_TAR, &tar);
		if (retval == ERROR_OK)
			retval = dap_run(dap);
		if (retval != ERROR_OK)
			return retval;
		/* Update TAR to reflect incremented address */
		dap->ap_tar_value = tar;
#if 0
		if (tar != address)
			LOG_ERROR("TAR auto-increment does not work");
#endif
	}

	/* Restore state */
	address = adr;
	nbytes = size * count;
	read_ptr = read_buf;

	/* If something failed, read TAR to find out how much data was successfully read. */
	if (retval != ERROR_OK) {
		uint32_t tar;
		if (dap_queue_ap_read(dap, AP_REG_TAR, &tar) == ERROR_OK
				&& dap_run(dap) == ERROR_OK)
			LOG_ERROR("Failed to read memory at 0x%08"PRIx32, tar);
		else
			LOG_ERROR("Failed to read memory and, additionally, failed to find out where");

		nbytes = 0;
	}

	/* Replay loop to populate caller's buffer from the correct word and byte lane */
	while (nbytes > 0) {
		uint32_t this_size = size;
#if 0
		if (addrinc && dap->packed_transfers && nbytes >= 4
				&& max_tar_block_size(dap->tar_autoincr_block, address) >= 4) {
			this_size = 4;
		}
#endif

		if (dap->ti_be_32_quirks) {
			switch (this_size) {
			case 4:
				*buffer++ = *read_ptr >> 8 * (3 - (address++ & 3));
				*buffer++ = *read_ptr >> 8 * (3 - (address++ & 3));
			case 2:
				*buffer++ = *read_ptr >> 8 * (3 - (address++ & 3));
			case 1:
				*buffer++ = *read_ptr >> 8 * (3 - (address++ & 3));
			}
		} else {
			switch (this_size) {
			case 4:
				*buffer++ = *read_ptr >> 8 * (address++ & 3);
				*buffer++ = *read_ptr >> 8 * (address++ & 3);
			case 2:
				*buffer++ = *read_ptr >> 8 * (address++ & 3);
			case 1:
				*buffer++ = *read_ptr >> 8 * (address++ & 3);
			}
		}

		read_ptr++;
		nbytes -= this_size;
	}

	free(read_buf);
	return retval;
}

/*--------------------------------------------------------------------*/
/*          Wrapping function with selection of AP                    */
/*--------------------------------------------------------------------*/
int mem_ap_sel_read_u32(struct adiv5_dap *swjdp, uint8_t ap,
		uint32_t address, uint32_t *value)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_read_u32(swjdp, address, value);
}

int mem_ap_sel_write_u32(struct adiv5_dap *swjdp, uint8_t ap,
		uint32_t address, uint32_t value)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_write_u32(swjdp, address, value);
}

int mem_ap_sel_read_atomic_u32(struct adiv5_dap *swjdp, uint8_t ap,
		uint32_t address, uint32_t *value)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_read_atomic_u32(swjdp, address, value);
}

int mem_ap_sel_write_atomic_u32(struct adiv5_dap *swjdp, uint8_t ap,
		uint32_t address, uint32_t value)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_write_atomic_u32(swjdp, address, value);
}

int mem_ap_sel_read_buf(struct adiv5_dap *swjdp, uint8_t ap,
		uint8_t *buffer, uint32_t size, uint32_t count, uint32_t address)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_read(swjdp, buffer, size, count, address, true);
}

int mem_ap_sel_write_buf(struct adiv5_dap *swjdp, uint8_t ap,
		const uint8_t *buffer, uint32_t size, uint32_t count, uint32_t address)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_write(swjdp, buffer, size, count, address, true);
}

int mem_ap_sel_read_buf_noincr(struct adiv5_dap *swjdp, uint8_t ap,
		uint8_t *buffer, uint32_t size, uint32_t count, uint32_t address)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_read(swjdp, buffer, size, count, address, false);
}

int mem_ap_sel_write_buf_noincr(struct adiv5_dap *swjdp, uint8_t ap,
		const uint8_t *buffer, uint32_t size, uint32_t count, uint32_t address)
{
	dap_ap_select(swjdp, ap);
	return mem_ap_write(swjdp, buffer, size, count, address, false);
}

/*--------------------------------------------------------------------------*/


#define DAP_POWER_DOMAIN_TIMEOUT (10)

/* FIXME don't import ... just initialize as
 * part of DAP transport setup
*/
extern const struct dap_ops jtag_dp_ops;

/*--------------------------------------------------------------------------*/

/**
 * Initialize a DAP.  This sets up the power domains, prepares the DP
 * for further use, and arranges to use AP #0 for all AP operations
 * until dap_ap-select() changes that policy.
 *
 * @param dap The DAP being initialized.
 *
 * @todo Rename this.  We also need an initialization scheme which account
 * for SWD transports not just JTAG; that will need to address differences
 * in layering.  (JTAG is useful without any debug target; but not SWD.)
 * And this may not even use an AHB-AP ... e.g. DAP-Lite uses an APB-AP.
 */
int ahbap_debugport_init(struct adiv5_dap *dap)
{
	/* check that we support packed transfers */
	uint32_t csw, cfg;
	int retval;

	LOG_DEBUG(" ");

	/* JTAG-DP or SWJ-DP, in JTAG mode
	 * ... for SWD mode this is patched as part
	 * of link switchover
	 */
	if (!dap->ops)
		dap->ops = &jtag_dp_ops;

	/* Default MEM-AP setup.
	 *
	 * REVISIT AP #0 may be an inappropriate default for this.
	 * Should we probe, or take a hint from the caller?
	 * Presumably we can ignore the possibility of multiple APs.
	 */
	dap->ap_current = !0;
	dap_ap_select(dap, 0);
	/* Make sure CTRLSEL bit is cleared */
	retval = dap_queue_dp_write(dap, DP_SELECT, 0);
	if (retval != ERROR_OK)
		return retval;

	dap->last_read = NULL;

	for (size_t i = 0; i < 10; i++) {
		/* DP initialization */

		dap->dp_bank_value = 0;

		retval = dap_queue_dp_read(dap, DP_CTRL_STAT, &dap->dp_ctrl_stat);
		if (retval != ERROR_OK)
			continue;

		/* If system or debug is powered down, power them up */
		if ((dap->dp_ctrl_stat & (CSYSPWRUPACK | CDBGPWRUPACK))
			!= (CSYSPWRUPACK | CDBGPWRUPACK)) {

			dap->dp_ctrl_stat = CSYSPWRUPREQ | CDBGPWRUPREQ;
			retval = dap_queue_dp_write(dap, DP_CTRL_STAT, dap->dp_ctrl_stat);
			if (retval != ERROR_OK)
				continue;

			/* Check that we have system and debug power domains activated */
			LOG_DEBUG("DAP: wait CSYSPWRUPACK | CDBGPWRUPACK");
			retval = dap_dp_poll_register(dap, DP_CTRL_STAT,
						      CSYSPWRUPACK | CDBGPWRUPACK,
						      CSYSPWRUPACK | CDBGPWRUPACK,
						      DAP_POWER_DOMAIN_TIMEOUT);
			if (retval != ERROR_OK)
				continue;

			/* Request debug reset */
			dap->dp_ctrl_stat |= CDBGRSTREQ;
			retval = dap_queue_dp_write(dap, DP_CTRL_STAT, dap->dp_ctrl_stat);
			if (retval != ERROR_OK)
				continue;

			/* Wait for debug reset ack */
			LOG_DEBUG("DAP: wait CDBGRSTACK");
			retval = dap_dp_poll_register(dap, DP_CTRL_STAT,
						      CSYSPWRUPACK | CDBGPWRUPACK | CSYSPWRUPACK,
						      CSYSPWRUPACK | CDBGPWRUPACK | CSYSPWRUPACK,
						      DAP_POWER_DOMAIN_TIMEOUT);
			if (retval != ERROR_OK)
				LOG_DEBUG("DAP: CDBGRSTACK is not set");
		}

		if (transport_is_swd())
			/* Clear STICKYORUN, STICKYCMP, STICKYERR, and WDATAERR in CTRL_STAT
			   by writing to ABORT register */
			retval = dap_queue_dp_write(dap, DP_ABORT,
						    STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
		else {
			/* Clear STICKYORUN, STICKYCMP, and STICKYERR */
			dap->dp_ctrl_stat = CDBGPWRUPREQ | CSYSPWRUPREQ;
			dap->dp_ctrl_stat |= SSTICKYORUN | SSTICKYCMP | SSTICKYERR;
			retval = dap_queue_dp_write(dap, DP_CTRL_STAT, dap->dp_ctrl_stat);
		}
		if (retval != ERROR_OK)
			continue;

		/* With debug power on we can activate OVERRUN checking */
		dap->dp_ctrl_stat = CDBGPWRUPREQ | CSYSPWRUPREQ | CORUNDETECT;
		retval = dap_queue_dp_write(dap, DP_CTRL_STAT, dap->dp_ctrl_stat);
		if (retval != ERROR_OK)
			continue;

		retval = dap_setup_accessport(dap, CSW_8BIT | CSW_ADDRINC_PACKED, 0);
		if (retval != ERROR_OK)
			continue;

		retval = dap_queue_ap_read(dap, AP_REG_CSW, &csw);
		if (retval != ERROR_OK)
			continue;

		retval = dap_queue_ap_read(dap, AP_REG_CFG, &cfg);
		if (retval != ERROR_OK)
			continue;

		retval = dap_run(dap);
		if (retval != ERROR_OK)
			continue;

		break;
	}

	if (retval != ERROR_OK)
		return retval;

	if (csw & CSW_ADDRINC_PACKED)
		dap->packed_transfers = true;
	else
		dap->packed_transfers = false;

	/* Packed transfers on TI BE-32 processors do not work correctly in
	 * many cases. */
	if (dap->ti_be_32_quirks)
		dap->packed_transfers = false;

	LOG_DEBUG("MEM_AP Packed Transfers: %s",
			dap->packed_transfers ? "enabled" : "disabled");

	/* The ARM ADI spec leaves implementation-defined whether unaligned
	 * memory accesses work, only work partially, or cause a sticky error.
	 * On TI BE-32 processors, reads seem to return garbage in some bytes
	 * and unaligned writes seem to cause a sticky error.
	 * TODO: it would be nice to have a way to detect whether unaligned
	 * operations are supported on other processors. */
	dap->unaligned_access_bad = dap->ti_be_32_quirks;

	LOG_DEBUG("MEM_AP CFG: large data %d, long address %d, big-endian %d",
			!!(cfg & 0x04), !!(cfg & 0x02), !!(cfg & 0x01));

	return ERROR_OK;
}

/* CID interpretation -- see ARM IHI 0029B section 3
 * and ARM IHI 0031A table 13-3.
 */
static const char *class_description[16] = {
	"Reserved", "ROM table", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "CoreSight component", "Reserved", "Peripheral Test Block",
	"Reserved", "OptimoDE DESS",
	"Generic IP component", "PrimeCell or System component"
};

static bool is_dap_cid_ok(uint32_t cid3, uint32_t cid2, uint32_t cid1, uint32_t cid0)
{
	return cid3 == 0xb1 && cid2 == 0x05
			&& ((cid1 & 0x0f) == 0) && cid0 == 0x0d;
}

/*
 * This function checks the ID for each access port to find the requested Access Port type
 */
int dap_find_ap(struct adiv5_dap *dap, enum ap_type type_to_find, uint8_t *ap_num_out)
{
	int ap;

	/* Maximum AP number is 255 since the SELECT register is 8 bits */
	for (ap = 0; ap <= 255; ap++) {

		/* read the IDR register of the Access Port */
		uint32_t id_val = 0;
		dap_ap_select(dap, ap);

		int retval = dap_queue_ap_read(dap, AP_REG_IDR, &id_val);
		if (retval != ERROR_OK)
			return retval;

		retval = dap_run(dap);

		/* IDR bits:
		 * 31-28 : Revision
		 * 27-24 : JEDEC bank (0x4 for ARM)
		 * 23-17 : JEDEC code (0x3B for ARM)
		 * 16    : Mem-AP
		 * 15-8  : Reserved
		 *  7-0  : AP Identity (1=AHB-AP 2=APB-AP 0x10=JTAG-AP)
		 */

		/* Reading register for a non-existant AP should not cause an error,
		 * but just to be sure, try to continue searching if an error does happen.
		 */
		if ((retval == ERROR_OK) &&                  /* Register read success */
			((id_val & 0x0FFF0000) == 0x04770000) && /* Jedec codes match */
			((id_val & 0xFF) == type_to_find)) {     /* type matches*/

			LOG_DEBUG("Found %s at AP index: %d (IDR=0x%08" PRIX32 ")",
						(type_to_find == AP_TYPE_AHB_AP)  ? "AHB-AP"  :
						(type_to_find == AP_TYPE_APB_AP)  ? "APB-AP"  :
						(type_to_find == AP_TYPE_JTAG_AP) ? "JTAG-AP" : "Unknown",
						ap, id_val);

			*ap_num_out = ap;
			return ERROR_OK;
		}
	}

	LOG_DEBUG("No %s found",
				(type_to_find == AP_TYPE_AHB_AP)  ? "AHB-AP"  :
				(type_to_find == AP_TYPE_APB_AP)  ? "APB-AP"  :
				(type_to_find == AP_TYPE_JTAG_AP) ? "JTAG-AP" : "Unknown");
	return ERROR_FAIL;
}

int dap_get_debugbase(struct adiv5_dap *dap, int ap,
			uint32_t *dbgbase, uint32_t *apid)
{
	uint32_t ap_old;
	int retval;

	/* AP address is in bits 31:24 of DP_SELECT */
	if (ap >= 256)
		return ERROR_COMMAND_SYNTAX_ERROR;

	ap_old = dap->ap_current;
	dap_ap_select(dap, ap);

	retval = dap_queue_ap_read(dap, AP_REG_BASE, dbgbase);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_queue_ap_read(dap, AP_REG_IDR, apid);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;

	dap_ap_select(dap, ap_old);

	return ERROR_OK;
}

int dap_lookup_cs_component(struct adiv5_dap *dap, int ap,
			uint32_t dbgbase, uint8_t type, uint32_t *addr, int32_t *idx)
{
	uint32_t ap_old;
	uint32_t romentry, entry_offset = 0, component_base, devtype;
	int retval;

	if (ap >= 256)
		return ERROR_COMMAND_SYNTAX_ERROR;

	*addr = 0;
	ap_old = dap->ap_current;
	dap_ap_select(dap, ap);

	do {
		retval = mem_ap_read_atomic_u32(dap, (dbgbase&0xFFFFF000) |
						entry_offset, &romentry);
		if (retval != ERROR_OK)
			return retval;

		component_base = (dbgbase & 0xFFFFF000)
			+ (romentry & 0xFFFFF000);

		if (romentry & 0x1) {
			uint32_t c_cid1;
			retval = mem_ap_read_atomic_u32(dap, component_base | 0xff4, &c_cid1);
			if (retval != ERROR_OK) {
				LOG_ERROR("Can't read component with base address 0x%" PRIx32
					  ", the corresponding core might be turned off", component_base);
				return retval;
			}
			if (((c_cid1 >> 4) & 0x0f) == 1) {
				retval = dap_lookup_cs_component(dap, ap, component_base,
							type, addr, idx);
				if (retval == ERROR_OK)
					break;
				if (retval != ERROR_TARGET_RESOURCE_NOT_AVAILABLE)
					return retval;
			}

			retval = mem_ap_read_atomic_u32(dap,
					(component_base & 0xfffff000) | 0xfcc,
					&devtype);
			if (retval != ERROR_OK)
				return retval;
			if ((devtype & 0xff) == type) {
				if (!*idx) {
					*addr = component_base;
					break;
				} else
					(*idx)--;
			}
		}
		entry_offset += 4;
	} while (romentry > 0);

	dap_ap_select(dap, ap_old);

	if (!*addr)
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;

	return ERROR_OK;
}

static int dap_rom_display(struct command_context *cmd_ctx,
				struct adiv5_dap *dap, int ap, uint32_t dbgbase, int depth)
{
	int retval;
	uint32_t cid0, cid1, cid2, cid3, memtype, romentry;
	uint16_t entry_offset;
	char tabs[7] = "";

	if (depth > 16) {
		command_print(cmd_ctx, "\tTables too deep");
		return ERROR_FAIL;
	}

	if (depth)
		snprintf(tabs, sizeof(tabs), "[L%02d] ", depth);

	/* bit 16 of apid indicates a memory access port */
	if (dbgbase & 0x02)
		command_print(cmd_ctx, "\t%sValid ROM table present", tabs);
	else
		command_print(cmd_ctx, "\t%sROM table in legacy format", tabs);

	/* Now we read ROM table ID registers, ref. ARM IHI 0029B sec  */
	retval = mem_ap_read_u32(dap, (dbgbase&0xFFFFF000) | 0xFF0, &cid0);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(dap, (dbgbase&0xFFFFF000) | 0xFF4, &cid1);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(dap, (dbgbase&0xFFFFF000) | 0xFF8, &cid2);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(dap, (dbgbase&0xFFFFF000) | 0xFFC, &cid3);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(dap, (dbgbase&0xFFFFF000) | 0xFCC, &memtype);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;

	if (!is_dap_cid_ok(cid3, cid2, cid1, cid0))
		command_print(cmd_ctx, "\t%sCID3 0x%02x"
				", CID2 0x%02x"
				", CID1 0x%02x"
				", CID0 0x%02x",
				tabs,
				(unsigned)cid3, (unsigned)cid2,
				(unsigned)cid1, (unsigned)cid0);
	if (memtype & 0x01)
		command_print(cmd_ctx, "\t%sMEMTYPE system memory present on bus", tabs);
	else
		command_print(cmd_ctx, "\t%sMEMTYPE system memory not present: dedicated debug bus", tabs);

	/* Now we read ROM table entries from dbgbase&0xFFFFF000) | 0x000 until we get 0x00000000 */
	for (entry_offset = 0; ; entry_offset += 4) {
		retval = mem_ap_read_atomic_u32(dap, (dbgbase&0xFFFFF000) | entry_offset, &romentry);
		if (retval != ERROR_OK)
			return retval;
		command_print(cmd_ctx, "\t%sROMTABLE[0x%x] = 0x%" PRIx32 "",
				tabs, entry_offset, romentry);
		if (romentry & 0x01) {
			uint32_t c_cid0, c_cid1, c_cid2, c_cid3;
			uint32_t c_pid0, c_pid1, c_pid2, c_pid3, c_pid4;
			uint32_t component_base;
			uint32_t part_num;
			const char *type, *full;

			component_base = (dbgbase & 0xFFFFF000) + (romentry & 0xFFFFF000);

			/* IDs are in last 4K section */
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFE0, &c_pid0);
			if (retval != ERROR_OK) {
				command_print(cmd_ctx, "\t%s\tCan't read component with base address 0x%" PRIx32
					      ", the corresponding core might be turned off", tabs, component_base);
				continue;
			}
			c_pid0 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFE4, &c_pid1);
			if (retval != ERROR_OK)
				return retval;
			c_pid1 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFE8, &c_pid2);
			if (retval != ERROR_OK)
				return retval;
			c_pid2 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFEC, &c_pid3);
			if (retval != ERROR_OK)
				return retval;
			c_pid3 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFD0, &c_pid4);
			if (retval != ERROR_OK)
				return retval;
			c_pid4 &= 0xff;

			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFF0, &c_cid0);
			if (retval != ERROR_OK)
				return retval;
			c_cid0 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFF4, &c_cid1);
			if (retval != ERROR_OK)
				return retval;
			c_cid1 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFF8, &c_cid2);
			if (retval != ERROR_OK)
				return retval;
			c_cid2 &= 0xff;
			retval = mem_ap_read_atomic_u32(dap, component_base + 0xFFC, &c_cid3);
			if (retval != ERROR_OK)
				return retval;
			c_cid3 &= 0xff;

			command_print(cmd_ctx, "\t\tComponent base address 0x%" PRIx32 ", "
				      "start address 0x%" PRIx32, component_base,
				      /* component may take multiple 4K pages */
				      (uint32_t)(component_base - 0x1000*(c_pid4 >> 4)));
			command_print(cmd_ctx, "\t\tComponent class is 0x%" PRIx8 ", %s",
					(uint8_t)((c_cid1 >> 4) & 0xf),
					/* See ARM IHI 0029B Table 3-3 */
					class_description[(c_cid1 >> 4) & 0xf]);

			/* CoreSight component? */
			if (((c_cid1 >> 4) & 0x0f) == 9) {
				uint32_t devtype;
				unsigned minor;
				const char *major = "Reserved", *subtype = "Reserved";

				retval = mem_ap_read_atomic_u32(dap,
						(component_base & 0xfffff000) | 0xfcc,
						&devtype);
				if (retval != ERROR_OK)
					return retval;
				minor = (devtype >> 4) & 0x0f;
				switch (devtype & 0x0f) {
				case 0:
					major = "Miscellaneous";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 4:
						subtype = "Validation component";
						break;
					}
					break;
				case 1:
					major = "Trace Sink";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Port";
						break;
					case 2:
						subtype = "Buffer";
						break;
					case 3:
						subtype = "Router";
						break;
					}
					break;
				case 2:
					major = "Trace Link";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Funnel, router";
						break;
					case 2:
						subtype = "Filter";
						break;
					case 3:
						subtype = "FIFO, buffer";
						break;
					}
					break;
				case 3:
					major = "Trace Source";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Processor";
						break;
					case 2:
						subtype = "DSP";
						break;
					case 3:
						subtype = "Engine/Coprocessor";
						break;
					case 4:
						subtype = "Bus";
						break;
					case 6:
						subtype = "Software";
						break;
					}
					break;
				case 4:
					major = "Debug Control";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Trigger Matrix";
						break;
					case 2:
						subtype = "Debug Auth";
						break;
					case 3:
						subtype = "Power Requestor";
						break;
					}
					break;
				case 5:
					major = "Debug Logic";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Processor";
						break;
					case 2:
						subtype = "DSP";
						break;
					case 3:
						subtype = "Engine/Coprocessor";
						break;
					case 4:
						subtype = "Bus";
						break;
					case 5:
						subtype = "Memory";
						break;
					}
					break;
				case 6:
					major = "Perfomance Monitor";
					switch (minor) {
					case 0:
						subtype = "other";
						break;
					case 1:
						subtype = "Processor";
						break;
					case 2:
						subtype = "DSP";
						break;
					case 3:
						subtype = "Engine/Coprocessor";
						break;
					case 4:
						subtype = "Bus";
						break;
					case 5:
						subtype = "Memory";
						break;
					}
					break;
				}
				command_print(cmd_ctx, "\t\tType is 0x%02" PRIx8 ", %s, %s",
						(uint8_t)(devtype & 0xff),
						major, subtype);
				/* REVISIT also show 0xfc8 DevId */
			}

			if (!is_dap_cid_ok(cid3, cid2, cid1, cid0))
				command_print(cmd_ctx,
						"\t\tCID3 0%02x"
						", CID2 0%02x"
						", CID1 0%02x"
						", CID0 0%02x",
						(int)c_cid3,
						(int)c_cid2,
						(int)c_cid1,
						(int)c_cid0);
			command_print(cmd_ctx,
				"\t\tPeripheral ID[4..0] = hex "
				"%02x %02x %02x %02x %02x",
				(int)c_pid4, (int)c_pid3, (int)c_pid2,
				(int)c_pid1, (int)c_pid0);

			/* Part number interpretations are from Cortex
			 * core specs, the CoreSight components TRM
			 * (ARM DDI 0314H), CoreSight System Design
			 * Guide (ARM DGI 0012D) and ETM specs; also
			 * from chip observation (e.g. TI SDTI).
			 */
			part_num = (c_pid0 & 0xff);
			part_num |= (c_pid1 & 0x0f) << 8;
			switch (part_num) {
			case 0x000:
				type = "Cortex-M3 NVIC";
				full = "(Interrupt Controller)";
				break;
			case 0x001:
				type = "Cortex-M3 ITM";
				full = "(Instrumentation Trace Module)";
				break;
			case 0x002:
				type = "Cortex-M3 DWT";
				full = "(Data Watchpoint and Trace)";
				break;
			case 0x003:
				type = "Cortex-M3 FBP";
				full = "(Flash Patch and Breakpoint)";
				break;
			case 0x008:
				type = "Cortex-M0 SCS";
				full = "(System Control Space)";
				break;
			case 0x00a:
				type = "Cortex-M0 DWT";
				full = "(Data Watchpoint and Trace)";
				break;
			case 0x00b:
				type = "Cortex-M0 BPU";
				full = "(Breakpoint Unit)";
				break;
			case 0x00c:
				type = "Cortex-M4 SCS";
				full = "(System Control Space)";
				break;
			case 0x00d:
				type = "CoreSight ETM11";
				full = "(Embedded Trace)";
				break;
			/* case 0x113: what? */
			case 0x120:		/* from OMAP3 memmap */
				type = "TI SDTI";
				full = "(System Debug Trace Interface)";
				break;
			case 0x343:		/* from OMAP3 memmap */
				type = "TI DAPCTL";
				full = "";
				break;
			case 0x906:
				type = "Coresight CTI";
				full = "(Cross Trigger)";
				break;
			case 0x907:
				type = "Coresight ETB";
				full = "(Trace Buffer)";
				break;
			case 0x908:
				type = "Coresight CSTF";
				full = "(Trace Funnel)";
				break;
			case 0x910:
				type = "CoreSight ETM9";
				full = "(Embedded Trace)";
				break;
			case 0x912:
				type = "Coresight TPIU";
				full = "(Trace Port Interface Unit)";
				break;
			case 0x913:
				type = "Coresight ITM";
				full = "(Instrumentation Trace Macrocell)";
				break;
			case 0x914:
				type = "Coresight SWO";
				full = "(Single Wire Output)";
				break;
			case 0x917:
				type = "Coresight HTM";
				full = "(AHB Trace Macrocell)";
				break;
			case 0x920:
				type = "CoreSight ETM11";
				full = "(Embedded Trace)";
				break;
			case 0x921:
				type = "Cortex-A8 ETM";
				full = "(Embedded Trace)";
				break;
			case 0x922:
				type = "Cortex-A8 CTI";
				full = "(Cross Trigger)";
				break;
			case 0x923:
				type = "Cortex-M3 TPIU";
				full = "(Trace Port Interface Unit)";
				break;
			case 0x924:
				type = "Cortex-M3 ETM";
				full = "(Embedded Trace)";
				break;
			case 0x925:
				type = "Cortex-M4 ETM";
				full = "(Embedded Trace)";
				break;
			case 0x930:
				type = "Cortex-R4 ETM";
				full = "(Embedded Trace)";
				break;
			case 0x950:
				type = "CoreSight Component";
				full = "(unidentified Cortex-A9 component)";
				break;
			case 0x961:
				type = "CoreSight TMC";
				full = "(Trace Memory Controller)";
				break;
			case 0x962:
				type = "CoreSight STM";
				full = "(System Trace Macrocell)";
				break;
			case 0x9a0:
				type = "CoreSight PMU";
				full = "(Performance Monitoring Unit)";
				break;
			case 0x9a1:
				type = "Cortex-M4 TPUI";
				full = "(Trace Port Interface Unit)";
				break;
			case 0x9a5:
				type = "Cortex-A5 ETM";
				full = "(Embedded Trace)";
				break;
			case 0xc05:
				type = "Cortex-A5 Debug";
				full = "(Debug Unit)";
				break;
			case 0xc08:
				type = "Cortex-A8 Debug";
				full = "(Debug Unit)";
				break;
			case 0xc09:
				type = "Cortex-A9 Debug";
				full = "(Debug Unit)";
				break;
			case 0x4af:
				type = "Cortex-A15 Debug";
				full = "(Debug Unit)";
				break;
			default:
				LOG_DEBUG("Unrecognized Part number 0x%" PRIx32, part_num);
				type = "-*- unrecognized -*-";
				full = "";
				break;
			}
			command_print(cmd_ctx, "\t\tPart is %s %s",
					type, full);

			/* ROM Table? */
			if (((c_cid1 >> 4) & 0x0f) == 1) {
				retval = dap_rom_display(cmd_ctx, dap, ap, component_base, depth + 1);
				if (retval != ERROR_OK)
					return retval;
			}
		} else {
			if (romentry)
				command_print(cmd_ctx, "\t\tComponent not present");
			else
				break;
		}
	}
	command_print(cmd_ctx, "\t%s\tEnd of ROM table", tabs);
	return ERROR_OK;
}

static int dap_info_command(struct command_context *cmd_ctx,
		struct adiv5_dap *dap, int ap)
{
	int retval;
	uint32_t dbgbase, apid;
	int romtable_present = 0;
	uint8_t mem_ap;
	uint32_t ap_old;

	retval = dap_get_debugbase(dap, ap, &dbgbase, &apid);
	if (retval != ERROR_OK)
		return retval;

	ap_old = dap->ap_current;
	dap_ap_select(dap, ap);

	/* Now we read ROM table ID registers, ref. ARM IHI 0029B sec  */
	mem_ap = ((apid&0x10000) && ((apid&0x0F) != 0));
	command_print(cmd_ctx, "AP ID register 0x%8.8" PRIx32, apid);
	if (apid) {
		switch (apid&0x0F) {
			case 0:
				command_print(cmd_ctx, "\tType is JTAG-AP");
				break;
			case 1:
				command_print(cmd_ctx, "\tType is MEM-AP AHB");
				break;
			case 2:
				command_print(cmd_ctx, "\tType is MEM-AP APB");
				break;
			default:
				command_print(cmd_ctx, "\tUnknown AP type");
				break;
		}

		/* NOTE: a MEM-AP may have a single CoreSight component that's
		 * not a ROM table ... or have no such components at all.
		 */
		if (mem_ap)
			command_print(cmd_ctx, "AP BASE 0x%8.8" PRIx32, dbgbase);
	} else
		command_print(cmd_ctx, "No AP found at this ap 0x%x", ap);

	romtable_present = ((mem_ap) && (dbgbase != 0xFFFFFFFF));
	if (romtable_present)
		dap_rom_display(cmd_ctx, dap, ap, dbgbase, 0);
	else
		command_print(cmd_ctx, "\tNo ROM table present");
	dap_ap_select(dap, ap_old);

	return ERROR_OK;
}

COMMAND_HANDLER(handle_dap_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;
	uint32_t apsel;

	switch (CMD_ARGC) {
	case 0:
		apsel = dap->apsel;
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], apsel);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	return dap_info_command(CMD_CTX, dap, apsel);
}

COMMAND_HANDLER(dap_baseaddr_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t apsel, baseaddr;
	int retval;

	switch (CMD_ARGC) {
	case 0:
		apsel = dap->apsel;
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], apsel);
		/* AP address is in bits 31:24 of DP_SELECT */
		if (apsel >= 256)
			return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	dap_ap_select(dap, apsel);

	/* NOTE:  assumes we're talking to a MEM-AP, which
	 * has a base address.  There are other kinds of AP,
	 * though they're not common for now.  This should
	 * use the ID register to verify it's a MEM-AP.
	 */
	retval = dap_queue_ap_read(dap, AP_REG_BASE, &baseaddr);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;

	command_print(CMD_CTX, "0x%8.8" PRIx32, baseaddr);

	return retval;
}

COMMAND_HANDLER(dap_memaccess_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t memaccess_tck;

	switch (CMD_ARGC) {
	case 0:
		memaccess_tck = dap->memaccess_tck;
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], memaccess_tck);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	dap->memaccess_tck = memaccess_tck;

	command_print(CMD_CTX, "memory bus access delay set to %" PRIi32 " tck",
			dap->memaccess_tck);

	return ERROR_OK;
}

COMMAND_HANDLER(dap_apsel_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t apsel, apid;
	int retval;

	switch (CMD_ARGC) {
	case 0:
		apsel = 0;
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], apsel);
		/* AP address is in bits 31:24 of DP_SELECT */
		if (apsel >= 256)
			return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	dap->apsel = apsel;
	dap_ap_select(dap, apsel);

	retval = dap_queue_ap_read(dap, AP_REG_IDR, &apid);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;

	command_print(CMD_CTX, "ap %" PRIi32 " selected, identification register 0x%8.8" PRIx32,
			apsel, apid);

	return retval;
}

static int jim_dap_readmem(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *cmd_name = Jim_GetString(argv[0], NULL);

	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc - 1, argv + 1);

	if (goi.argc != 2) {
		Jim_SetResultFormatted(goi.interp,
				"usage: %s <ap> <address>", cmd_name);
		return JIM_ERR;
	}

	int e;
	jim_wide ap;
	e = Jim_GetOpt_Wide(&goi, &ap);
	if (e != JIM_OK)
		return e;

	jim_wide address;
	e = Jim_GetOpt_Wide(&goi, &address);
	if (e != JIM_OK)
		return e;

	/* all args must be consumed */
	if (goi.argc != 0)
		return JIM_ERR;

	struct command_context *cmd_ctx = current_command_context(goi.interp);
	struct target *target = get_current_target(cmd_ctx);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t value;
	int retval;

	retval = mem_ap_sel_read_atomic_u32(dap, (uint8_t)ap, (uint32_t)address, &value);
	if (retval != ERROR_OK)
		return JIM_ERR;

	Jim_SetResult(goi.interp, Jim_NewIntObj(goi.interp, value));

	return JIM_OK;
}

static int jim_dap_writemem(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *cmd_name = Jim_GetString(argv[0], NULL);

	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc - 1, argv + 1);

	if (goi.argc != 3) {
		Jim_SetResultFormatted(goi.interp,
				"usage: %s <ap> <address> <value>", cmd_name);
		return JIM_ERR;
	}

	int e;
	jim_wide ap;
	e = Jim_GetOpt_Wide(&goi, &ap);
	if (e != JIM_OK)
		return e;

	jim_wide address;
	e = Jim_GetOpt_Wide(&goi, &address);
	if (e != JIM_OK)
		return e;

	jim_wide value;
	e = Jim_GetOpt_Wide(&goi, &value);
	if (e != JIM_OK)
		return e;

	/* all args must be consumed */
	if (goi.argc != 0)
		return JIM_ERR;

	struct command_context *cmd_ctx = current_command_context(goi.interp);
	struct target *target = get_current_target(cmd_ctx);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	int retval;

	retval = mem_ap_sel_write_atomic_u32(dap, (uint8_t)ap, (uint32_t)address, (uint32_t)value);
	if (retval != ERROR_OK)
		return JIM_ERR;

	return JIM_OK;
}

COMMAND_HANDLER(dap_apcsw_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t apcsw = dap->apcsw[dap->apsel], sprot = 0;

	switch (CMD_ARGC) {
	case 0:
		command_print(CMD_CTX, "apsel %" PRIi32 " selected, csw 0x%8.8" PRIx32,
			(dap->apsel), apcsw);
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], sprot);
		/* AP address is in bits 31:24 of DP_SELECT */
		if (sprot > 1)
			return ERROR_COMMAND_SYNTAX_ERROR;
		if (sprot)
			apcsw |= CSW_SPROT;
		else
			apcsw &= ~CSW_SPROT;
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	dap->apcsw[dap->apsel] = apcsw;

	return 0;
}



COMMAND_HANDLER(dap_apid_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t apsel, apid;
	int retval;

	switch (CMD_ARGC) {
	case 0:
		apsel = dap->apsel;
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], apsel);
		/* AP address is in bits 31:24 of DP_SELECT */
		if (apsel >= 256)
			return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	dap_ap_select(dap, apsel);

	retval = dap_queue_ap_read(dap, AP_REG_IDR, &apid);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;

	command_print(CMD_CTX, "0x%8.8" PRIx32, apid);

	return retval;
}

COMMAND_HANDLER(dap_ti_be_32_quirks_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	uint32_t enable = dap->ti_be_32_quirks;

	switch (CMD_ARGC) {
	case 0:
		break;
	case 1:
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], enable);
		if (enable > 1)
			return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	dap->ti_be_32_quirks = enable;
	command_print(CMD_CTX, "TI BE-32 quirks mode %s",
		enable ? "enabled" : "disabled");

	return 0;
}

COMMAND_HANDLER(dap_dpread_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	int retval;
	uint8_t regnum;
	uint32_t regval;

	switch (CMD_ARGC) {
	case 1:
		COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], regnum);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	retval = dap_queue_dp_read(dap, regnum, &regval);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;
	command_print(CMD_CTX, "R DP reg 0x%02" PRIx8 " value 0x%08" PRIx32, regnum, regval);

	return retval;
}

COMMAND_HANDLER(dap_dpwrite_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	int retval;
	uint8_t regnum;
	uint32_t regval;

	switch (CMD_ARGC) {
	case 2:
		COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], regnum);
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], regval);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	retval = dap_queue_dp_write(dap, regnum, regval);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;
	command_print(CMD_CTX, "W DP reg 0x%02" PRIx8 " value 0x%08" PRIx32, regnum, regval);

	return retval;
}

COMMAND_HANDLER(dap_apread_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	int retval;
	uint8_t regnum;
	uint32_t regval;

	switch (CMD_ARGC) {
	case 1:
		COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], regnum);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	dap_ap_select(dap, dap->apsel);
	retval = dap_queue_ap_read(dap, regnum, &regval);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;
	command_print(CMD_CTX, "R AP reg 0x%02" PRIx8 " value 0x%08" PRIx32, regnum, regval);

	return retval;
}

COMMAND_HANDLER(dap_apwrite_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;

	int retval;
	uint8_t regnum;
	uint32_t regval;

	switch (CMD_ARGC) {
	case 2:
		COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], regnum);
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], regval);
		break;
	default:
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	dap_ap_select(dap, dap->apsel);
	retval = dap_queue_ap_write(dap, regnum, regval);
	if (retval != ERROR_OK)
		return retval;
	retval = dap_run(dap);
	if (retval != ERROR_OK)
		return retval;
	command_print(CMD_CTX, "W AP reg 0x%02" PRIx8 " value 0x%08" PRIx32, regnum, regval);

	return retval;
}

static const struct command_registration dap_commands[] = {
	{
		.name = "info",
		.handler = handle_dap_info_command,
		.mode = COMMAND_EXEC,
		.help = "display ROM table for MEM-AP "
			"(default currently selected AP)",
		.usage = "[ap_num]",
	},
	{
		.name = "apsel",
		.handler = dap_apsel_command,
		.mode = COMMAND_EXEC,
		.help = "Set the currently selected AP (default 0) "
			"and display the result",
		.usage = "[ap_num]",
	},
	{
		.name = "apcsw",
		.handler = dap_apcsw_command,
		.mode = COMMAND_EXEC,
		.help = "Set csw access bit ",
		.usage = "[sprot]",
	},

	{
		.name = "apid",
		.handler = dap_apid_command,
		.mode = COMMAND_EXEC,
		.help = "return ID register from AP "
			"(default currently selected AP)",
		.usage = "[ap_num]",
	},
	{
		.name = "baseaddr",
		.handler = dap_baseaddr_command,
		.mode = COMMAND_EXEC,
		.help = "return debug base address from MEM-AP "
			"(default currently selected AP)",
		.usage = "[ap_num]",
	},
	{
		.name = "memaccess",
		.handler = dap_memaccess_command,
		.mode = COMMAND_EXEC,
		.help = "set/get number of extra tck for MEM-AP memory "
			"bus access [0-255]",
		.usage = "[cycles]",
	},
	{
		.name = "readmem",
		.jim_handler = jim_dap_readmem,
		.mode = COMMAND_EXEC,
		.help = "read memory using MEM-AP",
		.usage = "ap address",
	},
	{
		.name = "writemem",
		.jim_handler = jim_dap_writemem,
		.mode = COMMAND_EXEC,
		.help = "write memory using MEM-AP",
		.usage = "ap address value",
	},
	{
		.name = "ti_be_32_quirks",
		.handler = dap_ti_be_32_quirks_command,
		.mode = COMMAND_CONFIG,
		.help = "set/get quirks mode for TI TMS450/TMS570 processors",
		.usage = "[enable]",
	},
	{
		.name = "dpread",
		.handler = dap_dpread_command,
		.mode = COMMAND_EXEC,
		.help = "read DP register",
		.usage = "[reg]",
	},
	{
		.name = "dpwrite",
		.handler = dap_dpwrite_command,
		.mode = COMMAND_EXEC,
		.help = "write DP register",
		.usage = "[reg] [value]",
	},
	{
		.name = "apread",
		.handler = dap_apread_command,
		.mode = COMMAND_EXEC,
		.help = "read AP register",
		.usage = "[reg]",
	},
	{
		.name = "apwrite",
		.handler = dap_apwrite_command,
		.mode = COMMAND_EXEC,
		.help = "write AP register",
		.usage = "[reg] [value]",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration dap_command_handlers[] = {
	{
		.name = "dap",
		.mode = COMMAND_EXEC,
		.help = "DAP command group",
		.usage = "",
		.chain = dap_commands,
	},
	COMMAND_REGISTRATION_DONE
};
