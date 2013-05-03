# Do "make printk" to make templates for the current e1000 driver, which
# calls printk directly to log events.  The resulting .to files should be
# copied to /var/log/evlog/templates/kern.
#
# If you have a driver that has been patched to use DPRINTK, then do
# "make DPRINTK".  The resulting .to files should be copied to
# /var/log/evlog/templates/kern.
#
# If you have a driver that has been patched to use netdev_printk, then do
# "make netdev_printk".  The resulting .to files should be copied to
# /var/log/evlog/templates/e1000.
include ../evlog_env
DRIVER=e1000

printk:
	../test_target.sh kern
	awk -f template.awk printk=1 netdev_printk=0 dev_printk=0 DPRINTK=0 \
		< $(DRIVER).pt > $(DRIVER).t
	/sbin/evltc -I.. -DFACILITY=\"kern\" $(DRIVER).t
	$(ELA_BIN_DIR)/ela_get_atts -f kern -d $(DRIVER) > $(DRIVER)_printk.rules

netdev_printk: 
	../test_target.sh e1000
	awk -f template.awk printk=0 netdev_printk=1 dev_printk=1 DPRINTK=0 \
		< $(DRIVER).pt > $(DRIVER).t
	/sbin/evltc -I.. -DFACILITY=\"e1000\" $(DRIVER).t
	$(ELA_BIN_DIR)/ela_get_atts -f e1000 -d $(DRIVER) > $(DRIVER)_netdev_printk.rules

DPRINTK: 
	../test_target.sh kern
	awk -f template.awk printk=0 netdev_printk=0 dev_printk=1 DPRINTK=1 \
		< $(DRIVER).pt > $(DRIVER).t
	/sbin/evltc -I.. -DFACILITY=\"kern\" $(DRIVER).t
	$(ELA_BIN_DIR)/ela_get_atts -f kern -d $(DRIVER) > $(DRIVER)_DPRINTK.rules

install_printk:
	cp *.to $(EVLOG_TEMPLATES_DIR)/kern
	cp $(DRIVER)_printk.rules $(ELA_RULES_DIR)

install_netdev_printk:
	mkdir -p $(EVLOG_TEMPLATES_DIR)/e1000
	cp *.to $(EVLOG_TEMPLATES_DIR)/e1000
	cp $(DRIVER)_netdev_printk.rules $(ELA_RULES_DIR)

install_DPRINTK:
	cp *.to $(EVLOG_TEMPLATES_DIR)/kern
	cp $(DRIVER)_DPRINTK.rules $(ELA_RULES_DIR)

clean:
	rm -f *.to
	rm -f *.rules

clobber: clean
	rm -f $(DRIVER).t
