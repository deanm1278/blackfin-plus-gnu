/***************************************************************************
 *   Copyright (C) 2009 by Marvell Technology Group Ltd.                   *
 *   Written by Nicolas Pitre <nico@marvell.com>                           *
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

#ifndef ARM_SEMIHOSTING_H
#define ARM_SEMIHOSTING_H

int arm_semihosting(struct target *target);
void arm_semihosting_step_over_svc(struct target *target);
void arm_semihosting_step_over_bkpt(struct target *target);
int arm_get_gdb_fileio_info(struct target *target, struct gdb_fileio_info *fileio_info);
int arm_gdb_fileio_end(struct target *target, int retcode, int fileio_errno, bool ctrl_c);
int arm_gdb_fileio_pre_write_buffer(struct target *target, uint32_t address, uint32_t size, const uint8_t *buffer);

#endif
