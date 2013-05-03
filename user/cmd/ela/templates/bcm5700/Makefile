include ../evlog_env
DRIVER=bcm5700
KFACILITY=kern

all: test_target $(DRIVER).to

test_target:
	../test_target.sh $(KFACILITY)

$(DRIVER).to: $(DRIVER).t
	/sbin/evltc -I.. $(DRIVER).t
	$(ELA_BIN_DIR)/ela_get_atts -f $(KFACILITY) -d $(DRIVER) > $(DRIVER).rules

$(DRIVER).t: $(DRIVER).pt
	awk -f ../template.awk printk=1 < $(DRIVER).pt > $(DRIVER).t

install:
	cp *.to $(EVLOG_TEMPLATES_DIR)/$(KFACILITY)
	cp *.rules $(ELA_RULES_DIR)

clean:
	rm -f *.to
	rm -f *.rules

clobber: clean
	rm -f $(DRIVER).t
