# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)

# Update version number.
AC_INIT([Enterprise Event Logging], [1.6.1], [bug-report@evlog.sf.net], 
	[evlog])

AC_CONFIG_SRCDIR([user/include/evlog.h.in])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AM_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AC_PROG_CPP
AC_PROG_LIBTOOL

# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [dlopen])
# FIXME: Replace `main' with a function in `-lnsl':
AC_CHECK_LIB([nsl], [yp_bind])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [pthread_join])
# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [compress])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h locale.h malloc.h netdb.h netinet/in.h paths.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h values.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_GETPGRP
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit bzero clock_gettime endgrent fdatasync ftruncate gethostname gettimeofday localtime_r memmove memset mkdir munmap putenv regcomp select setlocale socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strstr strtol strtoul sysinfo tzset])

# Setup default prefix
ac_default_prefix="/usr"

# Check the system header files directory
KERN_HEADER_INCLUDEDIR="/usr/include/linux"
AC_ARG_WITH(kernel-header-include,
    [  --with-kernel-header-include=DIR      directroy for kernel header files. [[default=/usr/include/linux]]],
    [ KERN_HEADER_INCLUDEDIR="$withval" ])
if
  test "X$KERN_HEADER_INCLUDEDIR" = X
then
  AC_MSG_ERROR(Could not locate kernel header directory)
fi
AC_SUBST(KERN_HEADER_INCLUDEDIR)

# Check system config
sysconfdir="/etc"
AC_ARG_WITH(sysconfdir,
    [  --with-sysconfdir=DIR directory for system configuration files. [[default=/var]]],
    [ sysconfdir="$withval" ])
if
  test "X$sysconfdir" = X
then
  AC_MSG_ERROR(Could not locate /etc directory)
fi
AC_SUBST(sysconfdir)

# Set up default localstatedir for evlog
localstatedir="/var/evlog"
AC_ARG_WITH(localstatedir,
    [  --with-localstatedir=DIR directroy. [[default=/var/evlog]]],
    [ localstatedir="$withval" ])
if
  test "X$localstatedir" = X
then
  AC_MSG_ERROR(Could not locate /var/evlog directory)
fi
AC_SUBST(localstatedir)
AC_DEFINE_UNQUOTED(EVLOG_STATE_DIR, "$localstatedir", [Where to keep run-time state])

# Setup default /sbindir
sbindir="/sbin"
AC_ARG_WITH(sbindir,
    [  --with-sbindir=DIR directroy. [[default=/sbin]]],
    [ sbindir="$withval" ])
if
  test "X$sbindir" = X
then
  AC_MSG_ERROR(Could not locate /sbin directory)
fi
AC_SUBST(sbindir)

# Setup default /optdir
optdir="/opt"
AC_ARG_WITH(optdir,
    [  --with-optdir=DIR directroy. [[default=/opt]]],
    [ optdir="$withval" ])
if
  test "X$optdir" = X
then
  AC_MSG_ERROR(Could not locate /opt directory)
fi
AC_SUBST(optdir)

# Setup default location of plugins
plugindir=${optdir}/evlog/plugins
AC_ARG_WITH(plugindir,
    [  --with-plugindir=DIR directroy. [[default=${optdir}/evlog/plugins]]],
    [ plugindir="$withval" ])
if
  test "X$plugindir" = X
then
  AC_MSG_ERROR(Could not locate plugin directory)
fi
AC_SUBST(plugindir)
AC_DEFINE_UNQUOTED(EVLOG_PLUGIN_DIR, "$plugindir", [Where to install plugins])

# Setup default location of plugin configuration
pluginconfdir=${sysconfdir}/evlog.d/plugins
AC_ARG_WITH(pluginconfdir,
    [  --with-pluginconfdir=DIR directroy. [[default=${sysconfdir}/evlog.d/plugins]]],
    [ pluginconfdir="$withval" ])
if
  test "X$pluginconfdir" = X
then
  AC_MSG_ERROR(Could not locate plugin configuration directory)
fi
AC_SUBST(pluginconfdir)
AC_DEFINE_UNQUOTED(EVLOG_PLUGIN_CONF_DIR, "$pluginconfdir", [Where to install plugin configuration files])

# Check the evlog configuration directory
EVLOG_CONF_DIR="${sysconfdir}/evlog.d"
AC_SUBST(EVLOG_CONF_DIR)


EVLOG_SHARE_DIR="/usr/share/evlog"
AC_SUBST(EVLOG_SHARE_DIR)

# Check the system cron job directory
CRONDIR="/etc/cron.d"
AC_ARG_WITH(cron-dir,
    [  --with-cron-dir=DIR      directroy for system cron job files. [[default=/etc/cron.d]]],
    [ CRONDIR="$withval" ])
if
  test "X$CRONDIR" = X
then
  AC_MSG_ERROR(Could not locate system cron job directory)
fi
AC_SUBST(CRONDIR)

# Check the init (rc) scripts directory
AC_MSG_CHECKING(which init (rc) directory to use)
INITDOTD_DIR=""
for initdir in /etc/init.d /etc/rc.d/init.d /sbin/init.d        \
         /usr/local/etc/rc.d /etc/rc.d
do
  if
    test -d $initdir
  then
    INITDOTD_DIR=$initdir
    AC_MSG_RESULT($INITDOTD_DIR);
    break
  fi
done
AC_ARG_WITH(initdir,
    [  --with-initdir=DIR      directroy for init (rc) scripts [[${INITDOTD_DIR}]]],
    [ if test x"$withval" = xprefix; then INITDOTD_DIR=${prefix}; else
         INITDOTD_DIR="$withval"; fi ])
if
  test "X$INITDOTD_DIR" = X
then
  AC_MSG_ERROR(Could not locate init directory)
fi
AC_SUBST(INITDOTD_DIR)

# Check kernel header location
KERNEL_HEADER_DIR="/usr/src/linux"

# Tarball file name after make dist
THE_TAR_FILE='evlog-${VERSION}.tar.gz'
#THE_TAR_FILE='${distdir}.tar.gz'
AC_SUBST(THE_TAR_FILE)

# Check the rpm building utility name
AC_MSG_CHECKING(which is used, rpmbuild or rpm?)
RPM_BUILD=""
for rpmbuild_bin in rpmbuild rpm
do
  for bindir in /bin /usr/bin /usr/local/bin        \
           /sbin /usr/sbin /usr/local/sbin
  do
    if
      test -x $bindir/$rpmbuild_bin
    then
      RPM_BUILD=$bindir/$rpmbuild_bin
      AC_MSG_RESULT($RPM_BUILD);
      break 2
    fi
  done
done
AC_SUBST(RPM_BUILD)

# For rpm spec file
ARCH=`rpm --showrc 2>/dev/null | (grep -e '^build arch' 2>/dev/null || echo build: noarch) |gawk -F': ' '{print $2}' 2>/dev/null || echo noarch`
AC_SUBST(ARCH)
AM_CONDITIONAL([PPC64], [test `uname -m` = ppc64])
RPM_NAME=evlog
AC_SUBST(RPM_NAME)
RPM_TCP_RMTLOG_BE_NAME=tcp_rmtlog_be
AC_SUBST(RPM_TCP_RMTLOG_BE_NAME)
RPM_UDP_RMTLOG_BE_NAME=udp_rmtlog_be
AC_SUBST(RPM_UDP_RMTLOG_BE_NAME)
RPM_PROJECT_DESC="Enterprise Event Logging"
AC_SUBST(RPM_PROJECT_DESC)
RPM_TCP_RMTLOG_BE_DESC="TCP Remote Logging Plugin"
AC_SUBST(RPM_TCP_RMTLOG_BE_DESC)
RPM_UDP_RMTLOG_BE_DESC="UDP Remote Logging Plugin"
AC_SUBST(RPM_UDP_RMTLOG_BE_DESC)
RPM_PACKAGER="Hien Nguyen <nguyhien@us.ibm.com>"
AC_SUBST(RPM_PACKAGER)
RPM_MAKE_OPTS="PPC_K64_U32=no"
AC_SUBST(RPM_MAKE_OPTS)
RPM_VERSION=$VERSION
AC_SUBST(RPM_VERSION)
RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)

AC_SUBST(ac_configure_args)

AC_CONFIG_FILES([evlog.spec \
		 Makefile \
		 user/Makefile \
		 user/cmd/Makefile \
		 user/cmd/ela/Makefile \
		 user/cmd/ela/ela_funcs \
		 user/cmd/ela/test/run_ela_tests.sh \
		 user/cmd/ela/test/ela_test1.sh \
		 user/cmd/ela/test/ela_test2.sh \
		 user/cmd/ela/test/Makefile \
		 user/cmd/ela/templates/Makefile \
		 user/cmd/evlactiond/Makefile \
		 user/cmd/evlconfig/Makefile \
		 user/cmd/evlfacility/Makefile \
		 user/cmd/evlfacility/test/Makefile \
		 user/cmd/evlgentmpls/Makefile \
		 user/cmd/evlnotify/Makefile \
		 user/cmd/evlnotify/test/Makefile \
		 user/cmd/evlnotify/test/test4.sh \
		 user/cmd/evlnotifyd/Makefile \
		 user/cmd/evlnotifyd/test/Makefile \
		 user/cmd/evlogd/Makefile \
		 user/cmd/evlogd/tcp_rmtlog_be/Makefile \
		 user/cmd/evlogd/tcp_rmtlog_be/tcp_rmtlog_be.spec \
		 user/cmd/evlogd/test/Makefile \
		 user/cmd/evlogd/udp_rmtlog_be/Makefile \
		 user/cmd/evlogd/udp_rmtlog_be/udp_rmtlog_be.spec \
		 user/cmd/evlogmgr/Makefile \
		 user/cmd/evlogmgr/test/Makefile \
		 user/cmd/evlsend/Makefile \
		 user/cmd/evltc/Makefile \
		 user/cmd/evlview/Makefile \
		 user/include/Makefile \
		 user/include/evlog.h \
		 user/include/linux/Makefile \
		 user/lib/Makefile \
		 user/lib/query/Makefile \
		 user/lib/template/Makefile \
		 user/lib/test/Makefile \
		 user/lib/test/bitfields/Makefile \
		 user/lib/test/facreg/Makefile \
		 user/lib/test/runtests.sh \
		 user/lib/util/Makefile \
		 user/libevlsyslog/Makefile \
		 user/man/Makefile \
		 user/man/evlconfig.1 \
		 user/man/evlfacility.1 \
		 user/man/evlgentmpls.1 \
		 user/man/evlnotify.1 \
		 user/man/evlog.1 \
		 user/man/evlogmgr.1 \
		 user/man/evlquery.1 \
		 user/man/evlremote.1 \
		 user/man/evlsend.1 \
		 user/man/evltc.1 \
		 user/man/evlview.1 \
		 user/sysfiles/Makefile \
		 user/sysfiles/etc/Makefile \
		 user/sysfiles/etc/init.d/Makefile \
		 user/sysfiles/etc/init.d/evlog \
		 ])

AC_OUTPUT

dnl *****************
dnl Configure summary
dnl *****************

eval my_datadir="`eval echo ${datadir}`"
eval my_includedir="`eval echo ${includedir}`"
eval my_sysinclude_prefix="${sysinclude_prefix}"
eval my_initdir="${INITDOTD_DIR}"
eval my_libdir="`eval echo ${libdir}`"
eval my_localstatedir="`eval echo ${localstatedir}`"
eval my_mandir="`eval echo ${mandir}`"
eval my_sbindir="`eval echo ${sbindir}`"
eval my_plugindir="`eval echo ${plugindir}`"
eval my_sysconfdir="`eval echo ${sysconfdir}`"
eval my_evlogconfdir="${EVLOG_CONF_DIR}"
eval my_evlogsharedir="${EVLOG_SHARE_DIR}"

AC_MSG_RESULT([])
AC_MSG_RESULT([$PACKAGE configuration:])
AC_MSG_RESULT([  Version                                = $VERSION])
AC_MSG_RESULT([  Kernel header directory                = $KERN_HEADER_INCLUDEDIR])
AC_MSG_RESULT([  evlog configuration directory          = $my_evlogconfdir])
AC_MSG_RESULT([  evlog plugins directory                = $my_plugindir])
AC_MSG_RESULT([  System Executables                     = $my_sbindir])
AC_MSG_RESULT([  Man pages                              = $my_mandir])
AC_MSG_RESULT([  Libraries                              = $my_libdir])
AC_MSG_RESULT([  Header files                           = $my_includedir])
AC_MSG_RESULT([  Data files                             = $my_datadir])
AC_MSG_RESULT([  State information                      = $my_localstatedir])
AC_MSG_RESULT([  System configuration                   = $my_sysconfdir])
AC_MSG_RESULT([  Init (rc) scripts                      = $my_initdir])
AC_MSG_RESULT([  Share directory                        = $my_evlogsharedir])

