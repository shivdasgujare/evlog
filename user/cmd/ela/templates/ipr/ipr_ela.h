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

#ifndef _IPR_ELA_H
#define _IPR_ELA_H

#define IPR_NAME "ipr"
/* Prefix added by ipr_sdev_printk */
#define ISDEV_PFX_FMT IPR_NAME ": %d:%d:%d:%d: "
#define ISDEV_PFX_ATTS int host_no; int channel; int id; int lun;
/* Prefix added by ipr_res_printk */
#define IRES_PFX_FMT IPR_NAME ": %d:%d:%d:%d: "
#define IRES_PFX_ATTS int host_no; int bus; int target; int lun;
/* Prefix added by ipr_err, ipr_dev, etc. */
#define IPR_PFX_FMT IPR_NAME ": "
#define IPR_PFX_ATTS

#endif /* _IPR_ELA_H */
