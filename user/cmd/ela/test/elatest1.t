#include "ela.h"

facility "e1000s";
event_type NPFX_FMT "The EEPROM Checksum Is Not Valid\n";
const {
	string eventName = "invalid_eeprom_cksum";
	string reportType = "MENU";
	string file = "drivers/net/e1000/e1000_main.c";
	string function = "e1000_probe";
	ElaClass class = HARDWARE;
	ElaType type = PERM;
	ElaStringList probableCauses = {
		"Unable to access PCI data area for checksum value.  However, device ID is valid."
	};
	ElaStringList actions = {
		"Verify driver software level",
		"Execute diagnostics",
		"Replace adapter"
	};
	int threshold = 1;
	string interval = "-1";
	string forany = "dev_name";
}

attributes {
	string fmt;
	int argsz;
	NPFX_ATTS
}
format string "%fmt:printk%"
END
