/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */

#ifndef _SCANNER_H_
#define _SCANNER_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int lxGetOctalEscape(const char *digits, int isCharConst,
	int(*input)(), void(*unput)(int));
extern int lxGetHexEscape(const char *digits, int(*input)(), void(*unput)(int));
extern int lxGetCharEscape(char c);
extern char *lxGetString(int quoted, int(*input)(), void(*unput)(int),
	int *lineNumber);

#ifdef __cplusplus
}
#endif

#endif /* _SCANNER_H_ */
