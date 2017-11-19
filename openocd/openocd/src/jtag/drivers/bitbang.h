/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 Øyvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
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

#ifndef BITBANG_H
#define BITBANG_H

#include <jtag/swd.h>

struct bitbang_interface {
	/* low level callbacks (for bitbang)
	 */
	int (*read)(void);
	void (*write)(int tck, int tms, int tdi);
	void (*reset)(int trst, int srst);
	void (*blink)(int on);
	int (*swdio_read)(void);
	void (*swdio_drive)(bool on);
};

const struct swd_driver bitbang_swd;

extern bool swd_mode;

int bitbang_execute_queue(void);

extern struct bitbang_interface *bitbang_interface;
void bitbang_switch_to_swd(void);
int bitbang_swd_switch_seq(struct adiv5_dap *dap, enum swd_special_seq seq);

#endif /* BITBANG_H */
