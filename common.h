#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <time.h>
#include <pwd.h>

#include <pthread.h>
#include <sys/types.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "nt_types.h"

#define SEM_SERVER_RUN "/sem_server_run"
#define SEM_QTWINDOW_RUN "/sem_qtwindow_run"

#define	SUCCEED		0
#define	FAIL		-1

#define LEVEL_WARNING	3
#define LEVEL_DEBUG		4

#define PHRASE 100
#define MAX_STRING_LEN		2048
#define MAX_BUFFER_LEN		65536
#define NT_MEBIBYTE		1048576
#define NT_MAX_UINT64		(~__UINT64_C(0))

#define SEC_PER_MIN		60
#define SEC_PER_HOUR		3600
#define SEC_PER_DAY		86400
#define SEC_PER_WEEK		(7 * SEC_PER_DAY)
#define SEC_PER_MONTH		(30 * SEC_PER_DAY)
#define SEC_PER_YEAR		(365 * SEC_PER_DAY)

#	define nt_int64_t	 int64_t
#	define nt_uint64_t	uint64_t
# define nt_uint     unsigned int
#	define un_int64_t	  int64_t

#define NT_PTR_SIZE		sizeof(void *)

#define NT_TASK_FLAG_MULTIPLE_AGENTS 0x01
#define NT_TASK_FLAG_FOREGROUND      0x02

#define NT_NULL2STR(str)	(NULL != str ? str : "(null)")
#define NT_NULL2EMPTY_STR(str)	(NULL != (str) ? (str) : "")

#ifndef MAX
#	define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#	define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct stat	nt_stat_t;
#	define nt_error __nt_nt_error
void	nt_error(const char *fmt, ...);

#define ARRSIZE(a)	(sizeof(a) / sizeof(*a))
#define NT_UNUSED(var) (void)(var)

typedef enum
{
	NT_TASK_START = 0,
	NT_TASK_PRINT_SUPPORTED,
	NT_TASK_TEST_METRIC,
	NT_TASK_SHOW_USAGE,
	NT_TASK_SHOW_VERSION,
	NT_TASK_SHOW_HELP,
	NT_TASK_RUNTIME_CONTROL
}
nt_task_t;

typedef struct
{
	nt_task_t	task;
	int		flags;
	int		data;
}NT_TASK_EX;

typedef struct
{
	char	tz_sign;	/* '+' or '-' */
	int	tz_hour;
	int	tz_min;
}
nt_timezone_t;

#define nt_fclose(file)	\
				\
do				\
{				\
	if (file)		\
	{			\
		fclose(file);	\
		file = NULL;	\
	}			\
}				\
while (0)

#define nt_free(ptr)		\
				\
do				\
{				\
	if (ptr)		\
	{			\
		free(ptr);	\
		ptr = NULL;	\
	}			\
}				\
while (0)

#define THIS_SHOULD_NEVER_HAPPEN	nt_error("ERROR [file:%s,line:%d] "				\
							"Something impossible has just happened.",	\
							__FILE__, __LINE__)

#if defined(__GNUC__) || defined(__clang__)
#	define __nt_attr_format_printf(idx1, idx2) __attribute__((__format__(__printf__, (idx1), (idx2))))
#else
#	define __nt_attr_format_printf(idx1, idx2)
#endif

size_t	__nt_nt_snprintf(char *str, size_t count, const char *fmt, ...);
size_t	nt_snprintf(char *str, size_t count, const char *fmt, ...);
#	define nt_snprintf __nt_nt_snprintf

#define strscpy(x, y)	nt_strlcpy(x, y, sizeof(x))
#define strscat(x, y)	nt_strlcat(x, y, sizeof(x))

#	define nt_dsprintf __nt_nt_dsprintf
#	define nt_strdcatf __nt_nt_strdcatf
char	*nt_dsprintf(char *dest, const char *f, ...);
size_t	nt_strlcpy(char *dst, const char *src, size_t siz);
void	nt_strlcat(char *dst, const char *src, size_t siz);
void	nt_get_time(struct tm *tm, long *milliseconds, nt_timezone_t *tz);
long	nt_get_timezone_offset(time_t t, struct tm *tm);
long int	nt_get_thread_id();

#define nt_calloc(old, nmemb, size)	nt_calloc2(__FILE__, __LINE__, old, nmemb, size)
#define nt_malloc(old, size)		nt_malloc2(__FILE__, __LINE__, old, size)
#define nt_realloc(src, size)		nt_realloc2(__FILE__, __LINE__, src, size)
#define nt_strdup(old, str)		nt_strdup2(__FILE__, __LINE__, old, str)

#define NT_STRDUP(var, str)	(var = nt_strdup(var, str))

void    *nt_calloc2(const char *filename, int line, void *old, size_t nmemb, size_t size);
void    *nt_malloc2(const char *filename, int line, void *old, size_t size);
void    *nt_realloc2(const char *filename, int line, void *old, size_t size);
char    *nt_strdup2(const char *filename, int line, char *old, const char *str);

char	*nt_strdcatf(char *dest, const char *f, ...);
char	*nt_dvsprintf(char *dest, const char *f, va_list args);
size_t	nt_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
void	*nt_guaranteed_memset(void *v, int c, size_t n);
void	nt_on_exit(void);
void	nt_alarm_flag_set(void);
#endif
