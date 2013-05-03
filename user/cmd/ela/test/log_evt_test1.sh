#!/bin/sh


# What you'd see from dev_printk/netdev_printk...
# Corresponds to sample2.t
DPFX_FMT='%s %s: '
DPFX_ATTS='e1000 0000:00:03.0'
NPFX_FMT='%s (%s %s) %s: '
NPFX_ATTS='eth0 e1000 0000:00:03.0'

/sbin/evlsend -f e1000s -s ERR -p "${NPFX_FMT}The EEPROM Checksum Is Not Valid" \
	$NPFX_ATTS PROBE
