## Process this file with automake to produce Makefile.in
                                                                                
include $(top_srcdir)/Makefile.vars
include $(top_srcdir)/Makefile.rules

DISTCLEANFILES +=
MAINTAINERCLEANFILES +=

EXTRA_DIST += $(ela_template_files) $(ela_ipr_template_files) \
		$(ela_pci_template_files) $(ela_e100_template_files) \
		$(ela_e1000_template_files) $(ela_emulex_template_files) \
		$(ela_bcm5700_template_files) $(ela_olympic_template_files) \
		$(ela_olympic_template_files) $(ela_pcnet32_template_files)
 
ela_template_files := ela.h evlog_env template.awk test_target.sh README 

ela_ipr_template_files := ipr/Makefile \
			ipr/ipr_rtable.awk \
			ipr/ipr_table.pt \
			ipr/ipr_opaque.t \
			ipr/ela_entry.t \
			ipr/ipr_table.awk \
			ipr/ipr.pt \
			ipr/ipr_template.awk \
			ipr/ipr_ela.h 

ela_pci_template_files := pci/Makefile pci/pci.pt

ela_e100_template_files := e100/Makefile e100/e100.pt e100/template.awk

ela_e1000_template_files := e1000/Makefile e1000/template.awk e1000/e1000.pt

ela_emulex_template_files := emulex/Makefile emulex/elx_opaque.t \
				emulex/elx.pt emulex/elx_template.awk

ela_bcm5700_template_files := bcm5700/Makefile bcm5700/bcm5700.pt

ela_olympic_template_files := olympic/Makefile olympic/olympic.pt

ela_pcnet32_template_files := pcnet32/Makefile pcnet32/pcnet32.pt			

ela_templatesdir := /usr/share/doc/packages/evlog/templates
ela_ipr_templatesdir := $(ela_templatesdir)/ipr
ela_pci_templatesdir := $(ela_templatesdir)/pci
ela_e100_templatesdir := $(ela_templatesdir)/e100
ela_e1000_templatesdir := $(ela_templatesdir)/e1000
ela_emulex_templatesdir := $(ela_templatesdir)/emulex
ela_bcm5700_templatesdir := $(ela_templatesdir)/bcm5700
ela_olympic_templatesdir := $(ela_templatesdir)/olympic
ela_pcnet32_templatesdir := $(ela_templatesdir)/pcnet32

ela_templates_SCRIPTS := $(ela_template_files)
ela_ipr_templates_SCRIPTS := $(ela_ipr_template_files)
ela_pci_templates_SCRIPTS := $(ela_pci_template_files)
ela_e100_templates_SCRIPTS := $(ela_e100_template_files)
ela_e1000_templates_SCRIPTS := $(ela_e1000_template_files)
ela_emulex_templates_SCRIPTS := $(ela_emulex_template_files)
ela_bcm5700_templates_SCRIPTS := $(ela_bcm5700_template_files)
ela_olympic_templates_SCRIPTS := $(ela_olympic_template_files)
ela_pcnet32_templates_SCRIPTS := $(ela_pcnet32_template_files)

SUBDIRS := .
