## Process this file with automake to produce Makefile.in

include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES +=
MAINTAINERCLEANFILES +=
INCLUDES = -I$(top_srcdir)/user/include -I$(top_srcdir)/user/include/linux

EXTRA_DIST += ela ela_crt_rpt ela_sig ela_funcs ela_console ela_hw_alert\
	      ela_show ela_show_all ela_vpd_funcs\
	      ela_remove ela_remove_all ipr_opaque_alert load_ela_rules.sh \
	      Makefile.mk
ela_action_scripts = ela ela_crt_rpt ela_sig ela_funcs ela_vpd_funcs ela_console ela_hw_alert ipr_opaque_alert load_ela_rules.sh

evlog_sharedir := @EVLOG_SHARE_DIR@
evlog_share_SCRIPTS := $(ela_action_scripts)
evlog_share_PROGRAMS := ela_fake_event

sbin_PROGRAMS := ela_add ela_get_atts ela_sig_send
sbin_SCRIPTS := ela_show ela_show_all ela_remove ela_remove_all

ela_add_SOURCES := ela_add.c
ela_add_CFLAGS :=

ela_sig_send_SOURCES := ela_sig_send.c
ela_sig_send_CFLAGS :=

ela_get_atts_SOURCES := ela_get_atts.c
ela_get_atts_LDADD := ../../lib/.libs/libevl.a

ela_fake_event_SOURCES := ela_fake_event.c
ela_fake_event_LDADD := ../../lib/.libs/libevl.a


SUBDIRS := . test templates

