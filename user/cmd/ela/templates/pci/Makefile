include ../evlog_env
KFACILITY=kern

all: test_target pci.to

test_target:
	../test_target.sh $(KFACILITY)

pci.to: pci.t
	/sbin/evltc -I.. pci.t
	$(ELA_BIN_DIR)/ela_get_atts -f $(KFACILITY) -d pci > pci.rules

pci.t: pci.pt
	awk -f ../template.awk < pci.pt > pci.t

install:
	cp *.to $(EVLOG_TEMPLATES_DIR)/$(KFACILITY)
	cp *.rules $(ELA_RULES_DIR)

clean:
	rm -f *.to
	rm -f *.rules

clobber: clean
	rm -f pci.t
