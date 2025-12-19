#include "common.h"

#include "nt_log.h"

extern const char	*progname;
extern pthread_t tid;
static NT_THREAD_LOCAL volatile sig_atomic_t	nt_timed_out;

long int nt_get_thread_id()
{
	return (long int)getpid();
}


void	nt_error(const char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);

	fprintf(stderr, "%s [%li]: ", progname, nt_get_thread_id());
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	fflush(stderr);

	va_end(args);
}

size_t	nt_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
	int	written_len = 0;

	if (0 < count)
	{
		if (0 > (written_len = vsnprintf(str, count, fmt, args)))
			written_len = (int)count - 1;		/* count an output error as a full buffer */
		else
			written_len = MIN(written_len, (int)count - 1);		/* result could be truncated */
	}
	str[written_len] = '\0';	/* always write '\0', even if buffer size is 0 or vsnprintf() error */

	return (size_t)written_len;
}


size_t	nt_snprintf(char *str, size_t count, const char *fmt, ...)
{
	size_t	written_len;
	va_list	args;

	va_start(args, fmt);
	written_len = nt_vsnprintf(str, count, fmt, args);
	va_end(args);

	return written_len;
}

size_t	nt_strlcpy(char *dst, const char *src, size_t siz)
{
	const char	*s = src;

	if (0 != siz)
	{
		while (0 != --siz && '\0' != *s)
			*dst++ = *s++;

		*dst = '\0';
	}

	return s - src;	/* count does not include null */
}

void	nt_strlcat(char *dst, const char *src, size_t siz)
{
	while ('\0' != *dst)
	{
		dst++;
		siz--;
	}

	nt_strlcpy(dst, src, siz);
}

void	nt_get_time(struct tm *tm, long *milliseconds, nt_timezone_t *tz)
{

	struct timeval	current_time;

	gettimeofday(&current_time, NULL);
	localtime_r(&current_time.tv_sec, tm);
	*milliseconds = current_time.tv_usec / 1000;
	if (NULL != tz)
	{
		long	offset;
	offset = nt_get_timezone_offset(current_time.tv_sec, tm);

	tz->tz_sign = (0 <= offset ? '+' : '-');
	tz->tz_hour = labs(offset) / SEC_PER_HOUR;
	tz->tz_min = (labs(offset) - tz->tz_hour * SEC_PER_HOUR) / SEC_PER_MIN;
	/* assuming no remaining seconds like in historic Asia/Riyadh87, Asia/Riyadh88 and Asia/Riyadh89 */
	}
}

static int	is_leap_year(int year)
{
	return 0 == year % 4 && (0 != year % 100 || 0 == year % 400) ? SUCCEED : FAIL;
}

long	nt_get_timezone_offset(time_t t, struct tm *tm)
{
	long		offset;
#ifndef HAVE_TM_TM_GMTOFF
	struct tm	tm_utc;
#endif

	*tm = *localtime(&t);

#ifdef HAVE_TM_TM_GMTOFF
	offset = tm->tm_gmtoff;
#else
	gmtime_r(&t, &tm_utc);

	offset = (tm->tm_yday - tm_utc.tm_yday) * SEC_PER_DAY +
			(tm->tm_hour - tm_utc.tm_hour) * SEC_PER_HOUR +
			(tm->tm_min - tm_utc.tm_min) * SEC_PER_MIN;	/* assuming seconds are equal */

	while (tm->tm_year > tm_utc.tm_year)
		offset += (SUCCEED == is_leap_year(tm_utc.tm_year++) ? SEC_PER_YEAR + SEC_PER_DAY : SEC_PER_YEAR);

	while (tm->tm_year < tm_utc.tm_year)
		offset -= (SUCCEED == is_leap_year(--tm_utc.tm_year) ? SEC_PER_YEAR + SEC_PER_DAY : SEC_PER_YEAR);
#endif

	return offset;
}

char	*nt_dvsprintf(char *dest, const char *f, va_list args)
{
	char	*string = NULL;
	int	n, size = MAX_STRING_LEN >> 1;

	va_list curr;

	while (1)
	{
		string = (char *)nt_malloc(string, size);

		va_copy(curr, args);
		n = vsnprintf(string, size, f, curr);
		va_end(curr);

		if (0 <= n && n < size)
			break;

		/* result was truncated */
		if (-1 == n)
			size = size * 3 / 2 + 1;	/* the length is unknown */
		else
			size = n + 1;	/* n bytes + trailing '\0' */

		nt_free(string);
	}

	nt_free(dest);

	return string;
}

char	*nt_dsprintf(char *dest, const char *f, ...)
{
	char	*string;
	va_list args;

	va_start(args, f);

	string = nt_dvsprintf(dest, f, args);

	va_end(args);

	return string;
}


char	*nt_strdcat(char *dest, const char *src)
{
	size_t	len_dest, len_src;

	if (NULL == src)
		return dest;

	if (NULL == dest)
		return nt_strdup(NULL, src);

	len_dest = strlen(dest);
	len_src = strlen(src);

	dest = (char *)nt_realloc(dest, len_dest + len_src + 1);

	nt_strlcpy(dest + len_dest, src, len_src + 1);

	return dest;
}

char	*nt_strdcatf(char *dest, const char *f, ...)
{
	char	*string, *result;
	va_list	args;

	va_start(args, f);
	string = nt_dvsprintf(NULL, f, args);
	va_end(args);

	result = nt_strdcat(dest, string);

	nt_free(string);

	return result;
}

void	*nt_calloc2(const char *filename, int line, void *old, size_t nmemb, size_t size)
{
	int	max_attempts;
	void	*ptr = NULL;

	/* old pointer must be NULL */
	if (NULL != old)
	{
		nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_calloc: allocating already allocated memory. "
				"Please report this to Phy developers.",
				filename, line);
	}

	for (
		max_attempts = 10, nmemb = MAX(nmemb, 1), size = MAX(size, 1);
		0 < max_attempts && NULL == ptr;
		ptr = calloc(nmemb, size), max_attempts--
	);

	if (NULL != ptr)
		return ptr;

	nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_calloc: out of memory. Requested " NT_FS_SIZE_T " bytes.",
			filename, line, (nt_fs_size_t)size);

	exit(EXIT_FAILURE);
}

void	*nt_malloc2(const char *filename, int line, void *old, size_t size)
{
	int	max_attempts;
	void	*ptr = NULL;

	/* old pointer must be NULL */
	if (NULL != old)
	{
		nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_malloc: allocating already allocated memory. "
				"Please report this to Phy developers.",
				filename, line);
	}

	for (
		max_attempts = 10, size = MAX(size, 1);
		0 < max_attempts && NULL == ptr;
		ptr = malloc(size), max_attempts--
	);

	if (NULL != ptr)
		return ptr;

	nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_malloc: out of memory. Requested " NT_FS_SIZE_T " bytes.",
			filename, line, (nt_fs_size_t)size);

	exit(EXIT_FAILURE);
}

void	*nt_realloc2(const char *filename, int line, void *old, size_t size)
{
	int	max_attempts;
	void	*ptr = NULL;

	for (
		max_attempts = 10, size = MAX(size, 1);
		0 < max_attempts && NULL == ptr;
		ptr = realloc(old, size), max_attempts--
	);

	if (NULL != ptr)
		return ptr;

	nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_realloc: out of memory. Requested " NT_FS_SIZE_T " bytes.",
			filename, line, (nt_fs_size_t)size);

	exit(EXIT_FAILURE);
}

char	*nt_strdup2(const char *filename, int line, char *old, const char *str)
{
	int	retry;
	char	*ptr = NULL;

	nt_free(old);

	for (retry = 10; 0 < retry && NULL == ptr; ptr = strdup(str), retry--)
		;

	if (NULL != ptr)
		return ptr;

	nt_log(LOG_LEVEL_CRIT, "[file:%s,line:%d] nt_strdup: out of memory. Requested " NT_FS_SIZE_T " bytes.",
			filename, line, (nt_fs_size_t)(strlen(str) + 1));

	exit(EXIT_FAILURE);
}

void	*nt_guaranteed_memset(void *v, int c, size_t n)
{
	volatile signed char	*p = (volatile signed char *)v;

	while (0 != n--)
		*p++ = (signed char)c;

	return v;
}

void	nt_alarm_flag_set(void)
{
	nt_timed_out = 1;
}

void	nt_free_service_resources(void)
{
	sigset_t	set;
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, NULL);
//	sem_close(p_sem_serv_run);
	sem_unlink(SEM_SERVER_RUN);
//	sem_close(p_sem_qwnd_run);
	sem_unlink(SEM_QTWINDOW_RUN);
}


void	nt_on_exit(void)
{
	pthread_cancel(tid);
    pthread_join(tid,NULL);
	printf("nt_on_exit() called\n");
	nt_free_service_resources();
	exit(EXIT_SUCCESS);
}
