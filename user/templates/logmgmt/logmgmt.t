typedef int evl_facreg_rq_status_t "%v/"
	"-1/kernel_failed/"
	"0/kernel_ok/"
	"1/already_registered/"
	"2/registered_ok/"
	"3/registration_failed/"
	"4/faccode_mismatch/"
	;

typedef unsigned int posix_log_facility_t "%#x";

struct evl_facreg_rq;
aligned attributes {
	posix_log_facility_t	fr_kernel_fac_code;
	posix_log_facility_t	fr_registry_fac_code;
	evl_facreg_rq_status_t	fr_rq_status;
}
format
facility code used by kernel = %fr_kernel_fac_code%
facility code stored in registry = %fr_registry_fac_code%
request status = %fr_rq_status%
END

facility "LOGMGMT"; event_type 40;
attributes {
	struct evl_facreg_rq	rq;
	string			facName;
}
format
Request to register facility with name = %facName%
%rq%
END
