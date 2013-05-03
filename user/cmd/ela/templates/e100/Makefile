include ../evlog_env
KFACILITY=kern
DRIVER=e100

all: test_target directories e100.to

test_target:
	../test_target.sh $(KFACILITY)

directories:
	mkdir -p $(KFACILITY)

$(DRIVER).t: $(DRIVER).pt template.awk
	awk -f template.awk FACILITY=$(KFACILITY) < $(DRIVER).pt > $(DRIVER).t

$(DRIVER).to: $(DRIVER).t ../ela.h
	/sbin/evltc -I.. $(DRIVER).t
	$(ELA_BIN_DIR)/ela_get_atts -f $(KFACILITY) -d $(DRIVER) > $(DRIVER).rules
	mv *.to $(KFACILITY)

install:
	cp $(KFACILITY)/*.to $(EVLOG_TEMPLATES_DIR)/$(KFACILITY)
	cp *.rules $(ELA_RULES_DIR)

clean:
	rm -f $(KFACILITY)/*.to
	rm -r $(DRIVER).rules

clobber: clean
	rm -f $(DRIVER).t
