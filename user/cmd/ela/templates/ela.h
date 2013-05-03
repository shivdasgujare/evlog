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

/* Header file used when compiling Error Log Analysis templates */

#ifndef _ELA_H
#define _ELA_H

typedef string ElaClass;
#define HARDWARE "HARDWARE"
#define SOFTWARE "SOFTWARE"
#define UNKNOWN "UNKNOWN"

typedef string ElaType;
#define PERM "PERM"
#define TEMP "TEMP"
#define CONFIG "CONFIG"
#define PEND "PEND"
#define PERF "PERF"
#define UNKN "UNKN"
#define INFO "INFO"

typedef string ElaStringList[] delimiter="\n";

/* Prefix added by dev_printk */
#define DPFX_FMT "%s %s: "
#define DPFX_ATTS string driver; string bus_id;
/* Prefix added by netdev_printk */
#define NPFX_FMT "%s (%s %s) %s: "
#define NPFX_ATTS string dev_name; string driver; string bus_id;

#endif /* _ELA_H */
