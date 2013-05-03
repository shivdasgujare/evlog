# KFACILITY is the facility code that comes out of the kernel.
# FACILITY is the subdirectory where we want the bulk of the templates to be.
# Since the lpfc driver now uses dev_info/dev_warn to log events, KFACILITY
# is lpfc.
include ../evlog_env
KFACILITY=lpfc
FACILITY=lpfc


all: test_target directories elx_opaque.to elx.to elx.rules

directories:
	mkdir -p $(KFACILITY)
	mkdir -p $(FACILITY)

test_target:
	../test_target.sh $(KFACILITY)

elx_opaque.to: elx_opaque.t
	/sbin/evltc -I.. -DKFACILITY=\"$(KFACILITY)\" -DFACILITY=\"$(FACILITY)\" elx_opaque.t
	basename *.to .to > event_type.real
	mv *.to $(KFACILITY)

elx.t: elx.pt elx_template.awk
	awk -f elx_template.awk facility=$(FACILITY) < elx.pt > elx.t

elx.to: elx.t ../ela.h
	/sbin/evltc -I.. elx.t
	mv *.to $(FACILITY)

elx.rules: elx.t ../ela.h
	(event_type=`cat event_type.real`; cd $(FACILITY); $(ELA_BIN_DIR)/ela_get_atts -f $(FACILITY) -a msgNum -e $$event_type > ../elx.rules) 

install:
	mkdir -p $(EVLOG_TEMPLATES_DIR)/$(FACILITY)
	cp $(FACILITY)/*.to $(EVLOG_TEMPLATES_DIR)/$(FACILITY)
	cp *.rules $(ELA_RULES_DIR)

clean:
	rm -f $(FACILITY)/*.to
	rm -f $(KFACILITY)/*.to
	rm -f event_type.real
	rm -f elx.rules

clobber: clean
	rm -f elx.t
