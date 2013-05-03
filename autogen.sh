#! /bin/sh
#
# Run all the stuff we need to generate the configure and Makefile.in
# in the automake process.
#
# The order needs to be that. If autoheader does not run after
# autoconf, it doesn't get all the symbols right. As automake requires
# config.h.in, created by autoheader, it needs to run after it.
#
# $Id: autogen.sh,v 1.1 2003/06/13 23:51:18 nguyhien Exp $


set -ex

aclocal
autoheader
# Set GNU strictness

libtoolize --automake --force
automake --foreign --add-missing --force-missing --include-deps

autoconf

# $Log: autogen.sh,v $
# Revision 1.1  2003/06/13 23:51:18  nguyhien
# autoconf
#
# Revision 1.1.1.1  2002/09/18 17:24:54  yzou
# initial importing for evlog subagent.
#
# Revision 1.1  2002/03/08 19:48:11  yzou
# adding new files. changing some directory structure.
#
# Revision 1.1  2002/02/26 02:23:25  yzou
# converting the package to use automake.
#
# Revision 1.3.2.1  2001/11/14 23:53:17  inaky
# Fix the invocation order.
#
# Revision 1.3  2001/11/10 02:48:03  inaky
# Removed old logs, now CVS keywords ain't expanded
#
