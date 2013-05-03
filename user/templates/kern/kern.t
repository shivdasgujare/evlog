/*
 * If your kernel is built to forward printk messages to evlog, and to
 * include the caller's location information (file, function, line) with
 * each printk**, then you should compile this template (evltc kern.t).
 * This template enables evlview to display the printk message and location
 * information in a legible format.  (If you don't like the format, change
 * the last line of this file and recompile.)
 *
 * ** In menuconfig, it's configured thus:
 * General setup  --->
 * [*] Enterprise event logging support
 * (nnn) Eventlog buffer size (in K bytes)
 * [*] Forward printk messages to enterprise event log
 * [*] Include caller information with printk records
 */
facility "kern";
event_type default;
attributes {
	string msg;
	string file;
	string function;
	int line;
}
format
%file%:%function%:%line%
%msg%
