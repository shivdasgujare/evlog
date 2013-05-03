/* typedefs used by struct stat */
#if defined(__s390x__) || defined(__ia64__)
typedef ulong dev_t;
#else
typedef ulonglong dev_t;
#endif

typedef ulong ino_t;

typedef uint mode_t;

#if defined(__s390x__) || defined(__ia64__) || defined(__x86_64__)
typedef ulong nlink_t;
#else
typedef uint nlink_t;
#endif

typedef uint uid_t;
typedef uint gid_t;
typedef long off_t;
typedef long blksize_t;
typedef long blkcnt_t;
typedef long time_t;
