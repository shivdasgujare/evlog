/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2002
 *
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
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "posix_evlog.h"
#include "posix_evlsup.h"

int main ()
{
	posix_log_facility_t facility;
	int type;
	posix_log_severity_t severity;

	facility = 128;			/* LOCAL0 */
	severity = 3;
	type = 2002;
	posix_log_printf(facility, type, severity, 0, "%s", "test duplicate");
	posix_log_printf(facility, type, severity, 0, "%s", "test duplicate");
	posix_log_printf(facility, type, severity, 0, "%s", "test duplicate");
	posix_log_printf(facility, type, severity, 0, "%s", "test duplicate");
	
	return 0;
}
