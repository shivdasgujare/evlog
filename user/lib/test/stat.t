#include "statdefs.h"

struct stat;
aligned attributes {
#if defined(__i386__) || defined(__PPC__)
	dev_t	st_dev;      /* device */
	ushort	__pad1;		/* sys/stat.h has this */
	ino_t	st_ino;      /* inode */
	mode_t	st_mode;     /* protection */
	nlink_t	st_nlink;    /* number of hard links */
	uid_t	st_uid;      /* user ID of owner */
	gid_t	st_gid;      /* group ID of owner */
	dev_t	st_rdev;     /* device type (if inode device) */
	ushort	__pad2;		/* sys/stat.h has this */
	off_t	st_size;     /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for filesystem I/O */
	blkcnt_t st_blocks;   /* number of blocks allocated */
	time_t	st_atime;    /* time of last access */
	ulong	__unused1;	/* sys/stat.h has this */
	time_t	st_mtime;    /* time of last modification */
	ulong	__unused2;	/* sys/stat.h has this */
	time_t	st_ctime;    /* time of last change */
	ulong	__unused3;	/* sys/stat.h has this */
	ulong	__unused4;	/* sys/stat.h has this */
	ulong	__unused5;	/* sys/stat.h has this */
#elif defined(__s390__) && !defined(__s390x__)
	dev_t	st_dev;      /* device */
	uint	__pad1;		/* sys/stat.h has this */
	ino_t	st_ino;      /* inode */
	mode_t	st_mode;     /* protection */
	nlink_t	st_nlink;    /* number of hard links */
	uid_t	st_uid;      /* user ID of owner */
	gid_t	st_gid;      /* group ID of owner */
	dev_t	st_rdev;     /* device type (if inode device) */
	uint	__pad2;		/* sys/stat.h has this */
	off_t	st_size;     /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for filesystem I/O */
	blkcnt_t st_blocks;   /* number of blocks allocated */
	time_t	st_atime;    /* time of last access */
	ulong	__unused1;	/* sys/stat.h has this */
	time_t	st_mtime;    /* time of last modification */
	ulong	__unused2;	/* sys/stat.h has this */
	time_t	st_ctime;    /* time of last change */
	ulong	__unused3;	/* sys/stat.h has this */
	ulong	__unused4;	/* sys/stat.h has this */
	ulong	__unused5;	/* sys/stat.h has this */
#elif defined(__x86_64__)
    dev_t st_dev;
    ino_t st_ino;
    nlink_t st_nlink;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    int pad0;
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
    time_t st_atime;
    long __reserved0;
    time_t st_mtime;
    long __reserved1;
    time_t st_ctime;
    long __reserved2;
    long __unused[3]	"(%lx )";
    ulong __unused4;
    ulong __unused5;
#else
	/* s390x or ia64 */
    dev_t st_dev;		 
    ino_t st_ino;		 
    nlink_t st_nlink;		 
    mode_t st_mode;		 
    uid_t st_uid;		 
    gid_t st_gid;		 
    int pad0;
    dev_t st_rdev;		 
    off_t st_size;		 
    time_t st_atime;		 
    long __reserved0;	 
    time_t st_mtime;		 
    long __reserved1;	 
    time_t st_ctime;		 
    long __reserved2;	 
    blksize_t st_blksize;	 
    blkcnt_t st_blocks;	 
    long __unused[3]	"(%lx )";
#endif
}
#undef HEX
#ifdef HEX
format string
"dev, ino = %st_dev:llx%, %st_ino:lx%\n"
"mode = %st_mode:x%\n"
"nlink = %st_nlink:x%\n"
"uid, gid = %st_uid:x%, %st_gid:x%\n"
"rdev = %st_rdev:llx%\n"
"size, blksize, blocks = %st_size:lx%, %st_blksize:lx%, %st_blocks:lx%\n"
"atime, mtime, ctime = %st_atime:lx%, %st_mtime:lx%, %st_ctime:lx%\n"
#else
format string
"dev, ino = %st_dev%, %st_ino%\n"
"mode = %st_mode:o%\n"
"nlink = %st_nlink%\n"
"uid, gid = %st_uid%, %st_gid%\n"
"rdev = %st_rdev%\n"
"size, blksize, blocks = %st_size%, %st_blksize%, %st_blocks%\n"
"atime, mtime, ctime = %st_atime%, %st_mtime%, %st_ctime%\n"
#endif
