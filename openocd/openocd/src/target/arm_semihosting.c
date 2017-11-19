/***************************************************************************
 *   Copyright (C) 2009 by Marvell Technology Group Ltd.                   *
 *   Written by Nicolas Pitre <nico@marvell.com>                           *
 *                                                                         *
 *   Copyright (C) 2010 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
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
 * Hold ARM semihosting support.
 *
 * Semihosting enables code running on an ARM target to use the I/O
 * facilities on the host computer. The target application must be linked
 * against a library that forwards operation requests by using the SVC
 * instruction trapped at the Supervisor Call vector by the debugger.
 * Details can be found in chapter 8 of DUI0203I_rvct_developer_guide.pdf
 * from ARM Ltd.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arm.h"
#include "armv4_5.h"
#include "arm7_9_common.h"
#include "armv7m.h"
#include "armv7a.h"
#include "cortex_m.h"
#include "register.h"
#include "arm_semihosting.h"
#include <helper/binarybuffer.h>
#include <helper/log.h>
#include <sys/stat.h>

/* We will need a temporary workspace for SYS_FLEN and SYS_TIME. */
#define ARM_SEMIHOSTING_TEMP_BUFFER_SIZE 128
static uint8_t arm_semihosting_temp_buffer[ARM_SEMIHOSTING_TEMP_BUFFER_SIZE];
static uint32_t arm_semihosting_work_addr;

/**
 * Checks for and processes an ARM semihosting request.  This is meant
 * to be called when the target is stopped due to a debug mode entry.
 * If the value 0 is returned then there was nothing to process. A non-zero
 * return value signifies that a request was processed and the target resumed,
 * or an error was encountered, in which case the caller must return
 * immediately.
 *
 * @param target Pointer to the ARM target to process.  This target must
 *	not represent an ARMv6-M or ARMv7-M processor.
 * @param retval Pointer to a location where the return code will be stored
 * @return non-zero value if a request was processed or an error encountered
 */
int arm_semihosting(struct target *target)
{
	struct arm *arm = target_to_arm(target);
	uint32_t pc, lr, spsr;
	struct reg *r;
	int retval;

	if (!arm->is_semihosting)
		return 0;

	if (is_arm7_9(target_to_arm7_9(target))
		|| is_armv7a(target_to_armv7a(target))) {
		uint32_t vector_base;

		if (arm->core_mode != ARM_MODE_SVC)
			return 0;

		/* MRC p15,0,<Rt>,c12,c0,0 ; Read Vector Base Register */
		retval = arm->mrc(target, 15,
				  0, 0,	/* op1, op2 */
				  12, 0,	/* CRn, CRm */
				  &vector_base);
		if (retval != ERROR_OK)
			return 1;

		/* Check Supervisor Call vector. */
		r = arm->pc;
		pc = buf_get_u32(r->value, 0, 32);
		/* TODO We should check DBGVCR.V */
		if (pc != vector_base + 0x8 && pc != 0xffff0008)
			return 0;

		r = arm_reg_current(arm, 14);
		lr = buf_get_u32(r->value, 0, 32);

		/* Core-specific code should make sure SPSR is retrieved
		 * when the above checks pass...
		 */
		if (!arm->spsr->valid) {
			LOG_ERROR("SPSR not valid!");
			return 0;
		}

		spsr = buf_get_u32(arm->spsr->value, 0, 32);

		/* check instruction that triggered this trap */
		if (spsr & (1 << 5)) {
			/* was in Thumb (or ThumbEE) mode */
			uint8_t insn_buf[2];
			uint16_t insn;

			retval = target_read_memory(target, lr-2, 2, 1, insn_buf);
			if (retval != ERROR_OK)
				return 0;
			insn = target_buffer_get_u16(target, insn_buf);

			/* SVC 0xab */
			if (insn != 0xDFAB)
				return 0;
		} else if (spsr & (1 << 24)) {
			/* was in Jazelle mode */
			return 0;
		} else {
			/* was in ARM mode */
			uint8_t insn_buf[4];
			uint32_t insn;

			retval = target_read_memory(target, lr-4, 4, 1, insn_buf);
			if (retval != ERROR_OK)
				return 0;
			insn = target_buffer_get_u32(target, insn_buf);

			/* SVC 0x123456 */
			if (insn != 0xEF123456)
				return 0;
		}
	} else if (is_armv7m(target_to_armv7m(target))) {
		uint16_t insn;

		if (target->debug_reason != DBG_REASON_BREAKPOINT)
			return 0;

		r = arm->pc;
		pc = buf_get_u32(r->value, 0, 32);

		pc &= ~1;
		retval = target_read_u16(target, pc, &insn);
		if (retval != ERROR_OK)
			return 0;

		/* bkpt 0xAB */
		if (insn != 0xBEAB)
			return 0;
	} else {
		LOG_ERROR("Unsupported semi-hosting Target");
		return 0;
	}

	return 1;
}

void arm_semihosting_step_over_svc(struct target *target)
{
	struct arm *arm = target_to_arm(target);

	if (is_arm7_9(target_to_arm7_9(target))
		|| is_armv7a(target_to_armv7a(target))) {
		uint32_t spsr, lr;

		/* LR --> PC */
		lr = buf_get_u32(arm_reg_current(arm, 14)->value, 0, 32);
		buf_set_u32(arm->core_cache->reg_list[15].value, 0, 32, lr);
		arm->core_cache->reg_list[15].dirty = 1;

		/* saved PSR --> current PSR */
		spsr = buf_get_u32(arm->spsr->value, 0, 32);

		/* REVISIT should this be arm_set_cpsr(arm, spsr)
		 * instead of a partially unrolled version?
		 */

		buf_set_u32(arm->cpsr->value, 0, 32, spsr);
		arm->cpsr->dirty = 1;
		arm->core_mode = spsr & 0x1f;
		if (spsr & 0x20)
			arm->core_state = ARM_STATE_THUMB;

	}
}

void arm_semihosting_step_over_bkpt(struct target *target)
{
	struct arm *arm = target_to_arm(target);

	if (is_armv7m(target_to_armv7m(target))) {
		uint32_t pc;

		/* resume execution, this will be pc+2 to skip over the
	 	* bkpt instruction */
		pc = buf_get_u32(arm->pc->value, 0, 32);
		buf_set_u32(arm->core_cache->reg_list[15].value, 0, 32, pc + 2);
		arm->core_cache->reg_list[15].dirty = 1;
	}
}

#define GDB_FILEIO_O_RDONLY	0x0
#define GDB_FILEIO_O_WRONLY	0x1
#define GDB_FILEIO_O_RDWR	0x2
#define GDB_FILEIO_O_APPEND	0x8
#define GDB_FILEIO_O_CREAT	0x200
#define GDB_FILEIO_O_TRUNC	0x400
#define GDB_FILEIO_O_EXCL	0x800

#define GDB_FILEIO_S_IFREG	0100000
#define GDB_FILEIO_S_IFDIR	040000
#define GDB_FILEIO_S_IRUSR	0400
#define GDB_FILEIO_S_IWUSR	0200
#define GDB_FILEIO_S_IXUSR	0100
#define GDB_FILEIO_S_IRGRP	040
#define GDB_FILEIO_S_IWGRP	020
#define GDB_FILEIO_S_IXGRP	010
#define GDB_FILEIO_S_IROTH	04
#define GDB_FILEIO_S_IWOTH	02
#define GDB_FILEIO_S_IXOTH	01

int arm_get_gdb_fileio_info(struct target *target, struct gdb_fileio_info *fileio_info)
{
	struct arm *arm = target_to_arm(target);
	uint32_t r0 = buf_get_u32(arm->core_cache->reg_list[0].value, 0, 32);
	uint32_t r1 = buf_get_u32(arm->core_cache->reg_list[1].value, 0, 32);
	uint8_t params[16];
	int retval;

	if (fileio_info == NULL) {
		LOG_ERROR("Target has not initial file-I/O data structure");
		return ERROR_FAIL;
	}

	if (!arm->hit_syscall)
		return ERROR_FAIL;

	arm->active_syscall_id = r0;

	LOG_DEBUG("hit syscall ID: 0x%x", r0);

	/* free previous identifier storage */
	if (NULL != fileio_info->identifier) {
		free(fileio_info->identifier);
		fileio_info->identifier = NULL;
	}

	switch (r0) {
	case 0x01:	/* SYS_OPEN */
		retval = target_read_memory(target, r1, 4, 3, params);
		if (retval != ERROR_OK)
			return retval;

		/* pointer to path string */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* length of path string */
		fileio_info->param_2 = target_buffer_get_u32(target, params+8) + 1;
		/* flags */
		fileio_info->param_3 = target_buffer_get_u32(target, params+4);
		/* mode */
		fileio_info->param_4 = 0;

		if (fileio_info->param_2 == 4) {
			uint8_t fn[4];
			int fd = -1;

			retval = target_read_memory(target, fileio_info->param_1, 1, 4, fn);
			if (retval != ERROR_OK)
				return retval;

			if (fn[0] == ':' && fn[1] == 't' && fn[2] == 't' && fn[3] == '\0') {
				if (fileio_info->param_3 == 0)
					fd = 0;
				else if (fileio_info->param_3 == 4)
					fd = 1;
				else if (fileio_info->param_3 == 8)
					fd = 2;
			}

			if (fd >= 0) {
				buf_set_u32(arm->core_cache->reg_list[0].value, 0, 32, fd);
				arm->core_cache->reg_list[0].dirty = 1;

				fileio_info->identifier = (char *)malloc(8);
				sprintf(fileio_info->identifier, "open012");
				break;
			}
		}

		uint32_t flags = fileio_info->param_3;
		fileio_info->param_3 = 0;
		if (flags & 2)
			fileio_info->param_3 |= GDB_FILEIO_O_RDWR;
		if (flags & 4)
			fileio_info->param_3 |= GDB_FILEIO_O_CREAT | GDB_FILEIO_O_TRUNC;
		if ((flags & 4) && !(flags & 2))
			fileio_info->param_3 |= GDB_FILEIO_O_WRONLY;
		if (flags & 8)
			fileio_info->param_3 |= GDB_FILEIO_O_APPEND;

		/* if O_CREAT, hard code mode to S_IRUSR | S_IWUSR */
		if (fileio_info->param_3 & GDB_FILEIO_O_CREAT)
			fileio_info->param_4 = GDB_FILEIO_S_IRUSR | GDB_FILEIO_S_IWUSR;

		fileio_info->identifier = malloc(5);
		sprintf(fileio_info->identifier, "open");
		break;

	case 0x02:	/* SYS_CLOSE */
		retval = target_read_memory(target, r1, 4, 1, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);

		fileio_info->identifier = malloc(6);
		sprintf(fileio_info->identifier, "close");
		break;

	case 0x03:	/* SYS_WRITEC */
		/* fd, use stdout */
		fileio_info->param_1 = 1;
		/* pointer to buffer */
		fileio_info->param_2 = r1;
		/* count */
		fileio_info->param_3 = 1;

		arm->semihosting_result = fileio_info->param_3;

		fileio_info->identifier = malloc(6);
		sprintf(fileio_info->identifier, "write");
		break;

	case 0x04:	/* SYS_WRITE0 */
		/* fd, use stdout */
		fileio_info->param_1 = 1;
		/* pointer to buffer */
		fileio_info->param_2 = r1;
		/* count */
		do {
			retval = target_read_memory(target, r1, 1, 1, params);
			if (retval != ERROR_OK)
				return retval;
			r1++;
		} while (params[0]);
		fileio_info->param_3 = r1 - fileio_info->param_2;

		arm->semihosting_result = fileio_info->param_3;

		fileio_info->identifier = malloc(6);
		sprintf(fileio_info->identifier, "write");
		break;

	case 0x05:	/* SYS_WRITE */
		retval = target_read_memory(target, r1, 4, 3, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* pointer to buffer */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4);
		/* count */
		fileio_info->param_3 = target_buffer_get_u32(target, params+8);

		arm->semihosting_result = fileio_info->param_3;

		fileio_info->identifier = malloc(6);
		sprintf(fileio_info->identifier, "write");
		break;

	case 0x06:	/* SYS_READ */
		retval = target_read_memory(target, r1, 4, 3, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* pointer to buffer */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4);
		/* count */
		fileio_info->param_3 = target_buffer_get_u32(target, params+8);

		arm->semihosting_result = fileio_info->param_3;

		fileio_info->identifier = malloc(5);
		sprintf(fileio_info->identifier, "read");
		break;

	case 0x09:	/* SYS_ISTTY */
		retval = target_read_memory(target, r1, 4, 1, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);

		fileio_info->identifier = (char *)malloc(7);
		sprintf(fileio_info->identifier, "isatty");
		break;

	case 0x0a:	/* SYS_SEEK */
		retval = target_read_memory(target, r1, 4, 2, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* offset */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4);
		/* flag */
		fileio_info->param_3 = SEEK_SET;

		fileio_info->identifier = (char *)malloc(6);
		sprintf(fileio_info->identifier, "lseek");
		break;

	case 0x0c:	/* SYS_FLEN */
		retval = target_read_memory(target, r1, 4, 1, params);
		if (retval != ERROR_OK)
			return retval;

		/* fd */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* buf */
		/* We use the stack for a temporary buffer. */
		arm_semihosting_work_addr = buf_get_u32(arm_reg_current(arm, ARM_SP)->value, 0, 32);
		arm_semihosting_work_addr -= ARM_SEMIHOSTING_TEMP_BUFFER_SIZE;
		retval = target_read_memory(target, arm_semihosting_work_addr, 1,
				ARM_SEMIHOSTING_TEMP_BUFFER_SIZE, arm_semihosting_temp_buffer);
		if (retval != ERROR_OK)
			return retval;

		fileio_info->param_2 = arm_semihosting_work_addr;
		fileio_info->identifier = (char *)malloc(6);
		sprintf(fileio_info->identifier, "fstat");
		break;

	case 0x0e:	/* SYS_REMOVE */
		retval = target_read_memory(target, r1, 4, 2, params);
		if (retval != ERROR_OK)
			return retval;

		/* pointer to path string */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* length of path string */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4) + 1;

		fileio_info->identifier = (char *)malloc(7);
		sprintf(fileio_info->identifier, "unlink");
		break;

	case 0x0f:	/* SYS_RENAME */
		retval = target_read_memory(target, r1, 4, 4, params);
		if (retval != ERROR_OK)
			return retval;

		/* pointer to old path string */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* length of old path string */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4) + 1;
		/* pointer to new path string */
		fileio_info->param_3 = target_buffer_get_u32(target, params+8);
		/* length of new path string */
		fileio_info->param_4 = target_buffer_get_u32(target, params+12) + 1;

		fileio_info->identifier = (char *)malloc(7);
		sprintf(fileio_info->identifier, "rename");
		break;

	case 0x11:	/* SYS_TIME */
		/* tv */
		/* We use the stack for a temporary buffer. */
		arm_semihosting_work_addr = buf_get_u32(arm_reg_current(arm, ARM_SP)->value, 0, 32);
		arm_semihosting_work_addr -= ARM_SEMIHOSTING_TEMP_BUFFER_SIZE;
		retval = target_read_memory(target, arm_semihosting_work_addr, 1,
				ARM_SEMIHOSTING_TEMP_BUFFER_SIZE, arm_semihosting_temp_buffer);
		if (retval != ERROR_OK)
			return retval;
		fileio_info->param_1 = arm_semihosting_work_addr;

		/* tz */
		fileio_info->param_2 = 0;

		fileio_info->identifier = (char *)malloc(13);
		sprintf(fileio_info->identifier, "gettimeofday");
		break;

	case 0x12:	/* SYS_SYSTEM */
		retval = target_read_memory(target, r1, 4, 2, params);
		if (retval != ERROR_OK)
			return retval;

		/* pointer to the command string */
		fileio_info->param_1 = target_buffer_get_u32(target, params+0);
		/* length of the command string */
		fileio_info->param_2 = target_buffer_get_u32(target, params+4) + 1;

		fileio_info->identifier = (char *)malloc(7);
		sprintf(fileio_info->identifier, "system");
		break;

	case 0x13:	/* SYS_ERRNO */
		buf_set_u32(arm->core_cache->reg_list[0].value, 0, 32, arm->semihosting_errno);
		arm->core_cache->reg_list[0].dirty = 1;

		fileio_info->identifier = (char *)malloc(6);
		sprintf(fileio_info->identifier, "errno");
		break;

	case 0x15:	/* SYS_GET_CMDLINE */
		retval = target_read_memory(target, r1, 4, 2, params);
		if (retval != ERROR_OK)
			return retval;
		else {
			uint32_t a = target_buffer_get_u32(target, params+0);
			uint32_t l = target_buffer_get_u32(target, params+4);
			char *arg = "foobar";
			uint32_t s = strlen(arg) + 1;
			if (l < s)
				return ERROR_FAIL;

			retval = target_write_buffer(target, a, s, (void *)arg);
			if (retval != ERROR_OK)
				return retval;

			buf_set_u32(arm->core_cache->reg_list[0].value, 0, 32, 0);
			arm->core_cache->reg_list[0].dirty = 1;

			fileio_info->identifier = (char *)malloc(16);
			sprintf(fileio_info->identifier, "get_commandline");
		}
		break;

	case 0x16:	/* SYS_HEAPINFO */
		retval = target_read_memory(target, r1, 4, 1, params);
		if (retval != ERROR_OK)
			return retval;
		else {
			uint32_t a = target_buffer_get_u32(target, params+0);
			target_buffer_set_u32(target, params, arm->heap_base);
			target_buffer_set_u32(target, params + 4, arm->heap_limit);
			target_buffer_set_u32(target, params + 8, arm->stack_base);
			target_buffer_set_u32(target, params + 12, arm->stack_limit);
			retval = target_write_memory(target, a, 4, 4, params);
			if (retval != ERROR_OK)
				return retval;

			buf_set_u32(arm->core_cache->reg_list[0].value, 0, 32, 0);
			arm->core_cache->reg_list[0].dirty = 1;

			fileio_info->identifier = (char *)malloc(9);
			sprintf(fileio_info->identifier, "heapinfo");
		}
		break;

	case 0x18:	/* angel_SWIreason_ReportException */
		fprintf(stderr, "semihosting: exception %#x\n", (unsigned) r1);

		switch (r1) {
		case 0x20026:	/* ADP_Stopped_ApplicationExit */
			/* exit status is not passed down */
			target->fileio_info->param_1 = 0;
			fileio_info->identifier = (char *)malloc(5);
			sprintf(fileio_info->identifier, "exit");
			return ERROR_OK;

		case 0x20001:	/* ADP_Stopped_UndefinedInstr */
			target->fileio_info->param_1 = 4 /* SIGILL */;
			break;

		case 0x20000:	/* ADP_Stopped_BranchThroughZero */
		case 0x20002:	/* ADP_Stopped_SoftwareInterrupt */
		case 0x20003:	/* ADP_Stopped_PrefetchAbort */
		case 0x20004:	/* ADP_Stopped_DataAbort */
		case 0x20005:	/* ADP_Stopped_AddressException */
		case 0x20006:	/* ADP_Stopped_IRQ */
		case 0x20007:	/* ADP_Stopped_FIQ */
		case 0x20020:	/* ADP_Stopped_BreakPoint */
		case 0x20021:	/* ADP_Stopped_WatchPoint */
		case 0x20022:	/* ADP_Stopped_StepComplete */
		case 0x20023:	/* ADP_Stopped_RunTimeError */
		case 0x20024:	/* ADP_Stopped_InternalError */
		case 0x20025:	/* ADP_Stopped_UserInterruption */
		case 0x20027:	/* ADP_Stopped_StackOverflow */
		case 0x20028:	/* ADP_Stopped_DivisionByZero */
		case 0x20029:	/* ADP_Stopped_OSSpecific */
		default:
			/* TODO find better signal for each cause */
			target->fileio_info->param_1 = 6 /* SIGABRT */;
			break;
		}

		fileio_info->identifier = (char *)malloc(7);
		sprintf(fileio_info->identifier, "signal");
		break;

	case 0x07:	/* SYS_READC */
	case 0x0d:	/* SYS_TMPNAM */
	case 0x10:	/* SYS_CLOCK */
	case 0x17:	/* angel_SWIreason_EnterSVC */
	default:
		fprintf(stderr, "semihosting: unsupported call %#x\n",
				(unsigned) r0);
		fileio_info->identifier = (char *)malloc(8);
		sprintf(fileio_info->identifier, "unknown");
		break;
	}

	return ERROR_OK;
}

int arm_gdb_fileio_end(struct target *target, int retcode, int fileio_errno, bool ctrl_c)
{
	uint32_t result;
	int retval;

	LOG_DEBUG("syscall return code: 0x%x, errno: 0x%x, ctrl_c: %s",
			retcode, fileio_errno, ctrl_c ? "true" : "false");

	struct arm *arm = target_to_arm(target);

	switch (arm->active_syscall_id) {
	case 0x05:	/* SYS_WRITE */
	case 0x06:	/* SYS_READ */
		if (retcode >= 0)
			result = arm->semihosting_result - retcode;
		else
			result = arm->semihosting_result;
		break;
	case 0x0c:	/* SYS_FLEN */
	case 0x11:	/* SYS_TIME */
		result = arm->semihosting_result;
		retval = target_write_memory(target, arm_semihosting_work_addr, 1,
				ARM_SEMIHOSTING_TEMP_BUFFER_SIZE, arm_semihosting_temp_buffer);
		if (retval != ERROR_OK)
			return retval;
		break;
	default:
		result = retcode;
	}

	buf_set_u32(arm->core_cache->reg_list[0].value, 0, 32, result);
	arm->core_cache->reg_list[0].dirty = 1;

	arm->semihosting_errno = fileio_errno;
	arm->semihosting_ctrl_c = ctrl_c;
	arm->active_syscall_id = 0;

	return ERROR_OK;
}


int arm_gdb_fileio_pre_write_buffer(struct target *target, uint32_t address,
		uint32_t size, const uint8_t *buffer)
{
	struct arm *arm = target_to_arm(target);

	if (arm->hit_syscall) {
		if (arm->active_syscall_id == 0x0c /* SYS_FLEN */) {
			/* If doing GDB file-I/O, target should convert 'struct stat'
			   from gdb-format to target-format.
			   And we are only interested in st_size. */
			/* st_size 4 */
			arm->semihosting_result = buffer[35] | (buffer[34] << 8) | (buffer[33] << 16) | (buffer[32] << 24);

			return ERROR_OK;
		} else if (arm->active_syscall_id == 0x11 /* SYS_TIME */) {
			/* If doing GDB file-I/O, target should convert 'struct timeval'
			   from gdb-format to target-format.
			   And we are only interested in tv_sec. */
			arm->semihosting_result = buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);

			return ERROR_OK;
		}
	}

	return ERROR_FAIL;
}
