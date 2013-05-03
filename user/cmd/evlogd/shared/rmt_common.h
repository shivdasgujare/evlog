#ifndef _RMT_COMMON_H_
#define _RMT_COMMON_H_

struct net_rechdr {
		unsigned int            log_magic;
        posix_log_recid_t   	log_recid;
        
        int             	log_format;
        int             	log_event_type;
        posix_log_facility_t 	log_facility;
        posix_log_severity_t    log_severity;
        uid_t           	log_uid;
        gid_t           	log_gid;
        pid_t           	log_pid;
        pid_t           	log_pgrp;
        unsigned int    	log_flags;
        posix_log_procid_t	log_processor;
        
    	long long          	log_size;    
		long long		log_time_tv_sec;
		long long		log_time_tv_nsec;
		unsigned long long	log_thread;
        
}; 

#define NET_REC_HDR_SIZE sizeof(struct net_rechdr)


void trim(const char * str);
int getConfigStringParam(const char * configFile, const char * name, char * value, size_t vbufsize);
int getConfigIntParam(const char * configFile, const char * name, int * value);
int hton_rechdr(struct posix_log_entry *src, struct net_rechdr *dest);
int ntoh_rechdr(struct net_rechdr *src, struct posix_log_entry *dest);
#endif /* _RMT_COMMON_H_ */
