#ifndef NT_TYPES_H
#define NT_TYPES_H
//#include "sysinc.h"
#include <stdint.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#	if !defined(NT_THREAD_LOCAL)
#		define NT_THREAD_LOCAL
#	endif


#	define nt_open(pathname, flags)	open(pathname, flags)
#	define PATH_SEPARATOR	'/'

#	define nt_stat(path, buf)		stat(path, buf)
#	define nt_fstat(fd, buf)		fstat(fd, buf)

#	define nt_uint64_t	 uint64_t
#	define NT_FS_UI64	"%lu"
#	define NT_FS_UO64	"%lo"
#	define NT_FS_UX64	"%lx"

#	define nt_int64_t	int64_t
#	define NT_FS_I64	"%ld"
#	define NT_FS_O64	"%lo"
#	define NT_FS_X64	"%lx"

typedef uint32_t	nt_uint32_t;

typedef off_t	nt_offset_t;
#	define nt_lseek(fd, offset, whence)	lseek(fd, (nt_offset_t)(offset), whence)

#define NT_FS_DBL		"%lf"
#define NT_FS_DBL_EXT(p)	"%." #p "lf"
#define NT_FS_DBL64		"%.17G"

#ifdef HAVE_ORACLE
#	define NT_FS_DBL64_SQL	NT_FS_DBL64 "d"
#else
#	define NT_FS_DBL64_SQL	NT_FS_DBL64
#endif

#define NT_PTR_SIZE		sizeof(void *)
#define NT_FS_SIZE_T		NT_FS_UI64
#define NT_FS_SSIZE_T		NT_FS_I64
#define NT_FS_TIME_T		NT_FS_I64
#define nt_fs_size_t		nt_uint64_t
#define nt_fs_ssize_t		nt_int64_t
#define nt_fs_time_t		nt_int64_t

#ifndef S_ISREG
#	define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#	define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

#define NT_STR2UINT64(uint, string) is_uint64(string, &uint)
#define NT_OCT2UINT64(uint, string) sscanf(string, NT_FS_UO64, &uint)
#define NT_HEX2UINT64(uint, string) sscanf(string, NT_FS_UX64, &uint)

#define NT_STR2UCHAR(var, string) var = (unsigned char)atoi(string)

#define NT_CONST_STRING(str) "" str
#define NT_CONST_STRLEN(str) (sizeof(NT_CONST_STRING(str)) - 1)

typedef struct
{
	nt_uint64_t	lo;
	nt_uint64_t	hi;
}
nt_uint128_t;

#define NT_SIZE_T_ALIGN8(size)	(((size) + 7) & ~(size_t)7)
#define NT_IS_TOP_BIT_SET(x)	(0 != ((__UINT64_C(1) << ((sizeof(x) << 3) - 1)) & (x)))
typedef struct nt_variant nt_variant_t;

#endif
