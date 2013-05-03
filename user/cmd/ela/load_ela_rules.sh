#!/bin/sh
ELA_SBIN_DIR=/usr/sbin
ELA_RULES_DIR=/etc/evlog.d/ela

$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/bcm5700.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/e100.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/e1000_printk.rules
#$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/e1000_netdev_printk.rules
#$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/e1000_DPRINTK.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/elx.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/ipr_main.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/ipr_sub.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/olympic.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/pcnet32.rules
$ELA_SBIN_DIR/ela_add -f $ELA_RULES_DIR/pci.rules


