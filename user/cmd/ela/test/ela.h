typedef string ElaClass;
#define HARDWARE "HARDWARE"
#define SOFTWARE "SOFTWARE"

typedef string ElaType;
#define PERM "PERM"
#define TEMP "TEMP"
#define CONFIG "CONFIG"
#define PEND "PEND"
#define PERF "PERF"
#define UNKN "UNKN"

typedef string ElaStringList[] delimiter="\n";

/* Prefix added by dev_printk */
#define DPFX_FMT "%s %s: "
#define DPFX_ATTS string driver; string bus_id;
/* Prefix added by netdev_printk */
#define NPFX_FMT "%s (%s %s) %s: "
#define NPFX_ATTS string dev_name; string driver; string bus_id; string msglvl;
