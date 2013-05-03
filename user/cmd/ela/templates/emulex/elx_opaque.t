/*
 * Linux Event Logging
 * Copyright (C) International Business Machines Corp., 2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Please send e-mail to kenistoj@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 */

#include "ela.h"

facility KFACILITY;
event_type DPFX_FMT "%d:%04d:%s\n";
attributes {
	string fmt;
	int argsz;
	DPFX_ATTS
	int board;
	int msgNum "%04d";
	string msg;
}
redirect { facility FACILITY; event_type msgNum; }
format string "%fmt:printk%"
END
