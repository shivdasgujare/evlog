# Set KFACILITY=ipr if you have installed the patches, provided by Jim K,
# that cause dev_printk, ipr_printk, ipr_sdev_printk, and ipr_res_printk
# to log to evlog.  (If you have done this, the various events will be
# logged to facility ipr.)
#
# If you have NOT installed the aforementioned patches, set KFACILITY=kern.
# (Without these patches, the various events will be logged to facility
# kern via printk forwarding.)
include ../evlog_env

KFACILITY=ipr

all: test_target directories ipr.to ipr_opaque.to ipr_dopq.to ipr_sopq.to

test_target:
	../test_target.sh ipr
	../test_target.sh ipr2

directories:
	mkdir -p kern ipr ipr2

ipr.t: ipr.pt ipr_template.awk
	awk -f ipr_template.awk facility=$(KFACILITY) < ipr.pt > ipr.t

ipr.to: ipr.t
	/sbin/evltc -I.. ipr.t
	$(ELA_BIN_DIR)/ela_get_atts -f $(KFACILITY) > ipr_main.rules
	mv *.to $(KFACILITY)

# obsolete
ipr_table.t: ipr_table.pt ipr_table.awk
	awk -f ipr_table.awk < ipr_table.pt > ipr_table.t

# obsolete
ipr_table.to: ipr_table.t
	/sbin/evltc -I.. ipr_table.t
	cp ela_entry.to ipr_table.to kern
	cp ela_entry.to ipr_table.to ipr

ipr_opaque.to: ipr_opaque.t
	/sbin/evltc -I.. -DFACILITY=\"$(KFACILITY)\" ipr_opaque.t
	mv *.to $(KFACILITY)

ipr_dopq.t: ipr_table.pt ipr_rtable.awk
	awk -f ipr_rtable.awk pkfunc=dev_printk facility=ipr \
		< ipr_table.pt > ipr_dopq.t

ipr_sopq.t: ipr_table.pt ipr_rtable.awk
	awk -f ipr_rtable.awk pkfunc=ipr_sdev_printk facility=ipr2 \
		< ipr_table.pt > ipr_sopq.t

ipr_dopq.to: ipr_dopq.t
	/sbin/evltc -I.. ipr_dopq.t
	mv *.to ipr

ipr_sopq.to: ipr_sopq.t
	/sbin/evltc -I.. ipr_sopq.t
	$(ELA_BIN_DIR)/ela_get_atts -f $(KFACILITY) -a msg> ipr_sub.rules
	mv *.to ipr2

install:
	mkdir -p $(EVLOG_TEMPLATES_DIR)/ipr
	mkdir -p $(EVLOG_TEMPLATES_DIR)/ipr2
	cp ipr/*.to $(EVLOG_TEMPLATES_DIR)/ipr
	cp ipr2/*.to $(EVLOG_TEMPLATES_DIR)/ipr2
	cp *.rules $(ELA_RULES_DIR)

clean:
	rm -f *.to kern/*.to ipr/*.to ipr2/*.to
	rm -r ipr_main.rules
	rm -r ipr_sub.rules

clobber: clean
	rm -f ipr.t ipr_table.t ipr_dopq.t ipr_sopq.t
