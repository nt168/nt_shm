#include "common.h"
#include "nt_log.h"
#include "nt_mutexs.h"
//#include "cfg.h"
#include "nt_pthreads.h"

static char		log_filename[MAX_STRING_LEN];
static int		log_type = LOG_TYPE_UNDEFINED;
static nt_mutex_t	log_access = NT_MUTEX_NULL;
int			nt_log_level = LOG_LEVEL_WARNING;

char	*CONFIG_LOG_FILE	= "/tmp/nt_base.log";
char	*CONFIG_LOG_TYPE_STR	= NULL;

//int	CONFIG_LOG_TYPE	= LOG_TYPE_UNDEFINED;
int	CONFIG_LOG_TYPE	= LOG_TYPE_FILE;
int	CONFIG_LOG_FILE_SIZE	= 1;

extern const char	syslog_app_name[];

#	define LOCK_LOG		lock_log()
#	define UNLOCK_LOG	unlock_log()


#define NT_MESSAGE_BUF_SIZE	1024

#	define NT_DEV_NULL	"/dev/null"

#ifndef _WINDOWS
const char	*nt_get_log_level_string(void)
{
	switch (nt_log_level)
	{
		case LOG_LEVEL_EMPTY:
			return "0 (none)";
		case LOG_LEVEL_CRIT:
			return "1 (critical)";
		case LOG_LEVEL_ERR:
			return "2 (error)";
		case LOG_LEVEL_WARNING:
			return "3 (warning)";
		case LOG_LEVEL_DEBUG:
			return "4 (debug)";
		case LOG_LEVEL_TRACE:
			return "5 (trace)";
	}

	THIS_SHOULD_NEVER_HAPPEN;
	exit(EXIT_FAILURE);
}

int	nt_increase_log_level(void)
{
	if (LOG_LEVEL_TRACE == nt_log_level)
		return FAIL;

	nt_log_level = nt_log_level + 1;

	return SUCCEED;
}

int	nt_decrease_log_level(void)
{
	if (LOG_LEVEL_EMPTY == nt_log_level)
		return FAIL;

	nt_log_level = nt_log_level - 1;

	return SUCCEED;
}
#endif

int	nt_redirect_stdio(const char *filename)
{
	const char	default_file[] = NT_DEV_NULL;
	int		open_flags = O_WRONLY, fd;

	if (NULL != filename && '\0' != *filename)
		open_flags |= O_CREAT | O_APPEND;
	else
		filename = default_file;

	if (-1 == (fd = open(filename, open_flags, 0666)))
	{
		nt_error("cannot open \"%s\": %s", filename, nt_strerror(errno));
		return FAIL;
	}

	fflush(stdout);
	if (-1 == dup2(fd, STDOUT_FILENO))
		nt_error("cannot redirect stdout to \"%s\": %s", filename, nt_strerror(errno));

	fflush(stderr);
	if (-1 == dup2(fd, STDERR_FILENO))
		nt_error("cannot redirect stderr to \"%s\": %s", filename, nt_strerror(errno));

	close(fd);

	if (-1 == (fd = open(default_file, O_RDONLY)))
	{
		nt_error("cannot open \"%s\": %s", default_file, nt_strerror(errno));
		return FAIL;
	}

	if (-1 == dup2(fd, STDIN_FILENO))
		nt_error("cannot redirect stdin to \"%s\": %s", default_file, nt_strerror(errno));

	close(fd);

	return SUCCEED;
}

static void	rotate_log(const char *filename)
{
	nt_stat_t		buf;
	nt_uint64_t		new_size;
	static nt_uint64_t	old_size = NT_MAX_UINT64; /* redirect stdout and stderr */

	if (0 != nt_stat(filename, &buf))
	{
		nt_redirect_stdio(filename);
		return;
	}

	new_size = buf.st_size;

	if (0 != CONFIG_LOG_FILE_SIZE && (nt_uint64_t)CONFIG_LOG_FILE_SIZE * NT_MEBIBYTE < new_size)
	{
		char	filename_old[MAX_STRING_LEN];

		strscpy(filename_old, filename);
		nt_strlcat(filename_old, ".old", MAX_STRING_LEN);
		remove(filename_old);

		if (0 != rename(filename, filename_old))
		{
			FILE	*log_file = NULL;

			if (NULL != (log_file = fopen(filename, "w")))
			{
				long		milliseconds;
				struct tm	tm;

				nt_get_time(&tm, &milliseconds, NULL);

				fprintf(log_file, "%6li:%.4d%.2d%.2d:%.2d%.2d%.2d.%03ld"
						" cannot rename log file \"%s\" to \"%s\": %s\n",
						nt_get_thread_id(),
						tm.tm_year + 1900,
						tm.tm_mon + 1,
						tm.tm_mday,
						tm.tm_hour,
						tm.tm_min,
						tm.tm_sec,
						milliseconds,
						filename,
						filename_old,
						nt_strerror(errno));

				fprintf(log_file, "%6li:%.4d%.2d%.2d:%.2d%.2d%.2d.%03ld"
						" Logfile \"%s\" size reached configured limit"
						" LogFileSize but moving it to \"%s\" failed. The logfile"
						" was truncated.\n",
						nt_get_thread_id(),
						tm.tm_year + 1900,
						tm.tm_mon + 1,
						tm.tm_mday,
						tm.tm_hour,
						tm.tm_min,
						tm.tm_sec,
						milliseconds,
						filename,
						filename_old);

				nt_fclose(log_file);

				new_size = 0;
			}
		}
		else
			new_size = 0;
	}

	if (old_size > new_size)
		nt_redirect_stdio(filename);

	old_size = new_size;
}

#ifndef _WINDOWS
static sigset_t	orig_mask;

static void	lock_log(void)
{
	sigset_t	mask;

	/* block signals to prevent deadlock on log file mutex when signal handler attempts to lock log */
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGHUP);

	if (0 > sigprocmask(SIG_BLOCK, &mask, &orig_mask))
		nt_error("cannot set sigprocmask to block the user signal");

	nt_mutex_lock(log_access);
}

static void	unlock_log(void)
{
	nt_mutex_unlock(log_access);

	if (0 > sigprocmask(SIG_SETMASK, &orig_mask, NULL))
		nt_error("cannot restore sigprocmask");
}
#else
static void	lock_log(void)
{
#ifdef NT_AGENT
	if (0 == (NT_MUTEX_LOGGING_DENIED & get_thread_global_mutex_flag()))
#endif
		LOCK_LOG;
}

static void	unlock_log(void)
{
#ifdef NT_AGENT
	if (0 == (NT_MUTEX_LOGGING_DENIED & get_thread_global_mutex_flag()))
#endif
		UNLOCK_LOG;
}
#endif

void	nt_handle_log(void)
{
	if (LOG_TYPE_FILE != log_type)
		return;

	LOCK_LOG;

	rotate_log(log_filename);

	UNLOCK_LOG;
}

int	nt_open_log(int type, int level, const char *filename, char **error)
{
	log_type = type;
	nt_log_level = level;

	if (LOG_TYPE_SYSTEM == type)
	{
		openlog(syslog_app_name, LOG_PID, LOG_DAEMON);
	}
	else if (LOG_TYPE_FILE == type)
	{
		FILE	*log_file = NULL;

		if (MAX_STRING_LEN <= strlen(filename))
		{
			*error = nt_strdup(*error, "too long path for logfile");
			return FAIL;
		}

		if (SUCCEED != nt_mutex_create(&log_access, NT_MUTEX_LOG, error))
			return FAIL;

		if (NULL == (log_file = fopen(filename, "a+")))
		{
			*error = nt_dsprintf(*error, "unable to open log file [%s]: %s", filename, nt_strerror(errno));
			return FAIL;
		}

		strscpy(log_filename, filename);
		nt_fclose(log_file);
	}
	else if (LOG_TYPE_CONSOLE == type || LOG_TYPE_UNDEFINED == type)
	{
		if (SUCCEED != nt_mutex_create(&log_access, NT_MUTEX_LOG, error))
		{
			*error = nt_strdup(*error, "unable to create mutex for standard output");
			return FAIL;
		}

		fflush(stderr);
		if (-1 == dup2(STDOUT_FILENO, STDERR_FILENO))
			nt_error("cannot redirect stderr to stdout: %s", nt_strerror(errno));
	}
	else
	{
		*error = nt_strdup(*error, "unknown log type");
		return FAIL;
	}

	return SUCCEED;
}

void	nt_close_log(void)
{
	if (LOG_TYPE_SYSTEM == log_type)
	{
		closelog();
	}
	else if (LOG_TYPE_FILE == log_type || LOG_TYPE_CONSOLE == log_type || LOG_TYPE_UNDEFINED == log_type)
	{
		nt_mutex_destroy(&log_access);
	}
}

void	__nt_nt_log(int level, const char *fmt, ...)
{
	char		message[MAX_BUFFER_LEN];
	va_list		args;
	char       title[PHRASE] = {0};

	switch(level)
	{
		case LOG_LEVEL_CRIT:
			nt_strlcat(title, "CRIT  ", PHRASE);
		break;
		case LOG_LEVEL_ERR:
			nt_strlcat(title, "ERR   ", PHRASE);
		break;
		case LOG_LEVEL_WARNING:
			nt_strlcat(title, "WARN  ", PHRASE);
		break;
		case LOG_LEVEL_DEBUG:
			nt_strlcat(title, "DEBUG ", PHRASE);
		break;
		case LOG_LEVEL_TRACE:
			nt_strlcat(title, "TRACE ", PHRASE);
		break;
		default:
			nt_strlcat(title, "UNKNOW", PHRASE);
		break;
    }
#ifndef NT_NT_LOG_CHECK
	if (SUCCEED != NT_CHECK_LOG_LEVEL(level))
		return;
#endif
	if (LOG_TYPE_FILE == log_type)
	{
		FILE	*log_file;

		LOCK_LOG;

		if (0 != CONFIG_LOG_FILE_SIZE)
			rotate_log(log_filename);

		if (NULL != (log_file = fopen(log_filename, "a+")))
		{
			long		milliseconds;
			struct tm	tm;

			nt_get_time(&tm, &milliseconds, NULL);

			fprintf(log_file,
					"[%s]%6li:%.4d%.2d%.2d:%.2d%.2d%.2d.%03ld ",
					title,
					nt_get_thread_id(),
					tm.tm_year + 1900,
					tm.tm_mon + 1,
					tm.tm_mday,
					tm.tm_hour,
					tm.tm_min,
					tm.tm_sec,
					milliseconds
					);

			va_start(args, fmt);
			vfprintf(log_file, fmt, args);
			va_end(args);

			fprintf(log_file, "\n");

			nt_fclose(log_file);
		}
		else
		{
			nt_error("failed to open log file: %s", nt_strerror(errno));

			va_start(args, fmt);
			nt_vsnprintf(message, sizeof(message), fmt, args);
			va_end(args);

			nt_error("failed to write [%s] into log file", message);
		}

		UNLOCK_LOG;

		return;
	}

	if (LOG_TYPE_CONSOLE == log_type)
	{
		long		milliseconds;
		struct tm	tm;

		LOCK_LOG;

		nt_get_time(&tm, &milliseconds, NULL);

		fprintf(stdout,
				"%6li:%.4d%.2d%.2d:%.2d%.2d%.2d.%03ld ",
				nt_get_thread_id(),
				tm.tm_year + 1900,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec,
				milliseconds
				);

		va_start(args, fmt);
		vfprintf(stdout, fmt, args);
		va_end(args);

		fprintf(stdout, "\n");

		fflush(stdout);

		UNLOCK_LOG;

		return;
	}

	va_start(args, fmt);
	nt_vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	if (LOG_TYPE_SYSTEM == log_type)
	{
		/* for nice printing into syslog */
		switch (level)
		{
			case LOG_LEVEL_CRIT:
				syslog(LOG_CRIT, "%s", message);
				break;
			case LOG_LEVEL_ERR:
				syslog(LOG_ERR, "%s", message);
				break;
			case LOG_LEVEL_WARNING:
				syslog(LOG_WARNING, "%s", message);
				break;
			case LOG_LEVEL_DEBUG:
			case LOG_LEVEL_TRACE:
				syslog(LOG_DEBUG, "%s", message);
				break;
			case LOG_LEVEL_INFORMATION:
				syslog(LOG_INFO, "%s", message);
				break;
			default:
				/* LOG_LEVEL_EMPTY - print nothing */
				break;
		}
	}	/* LOG_TYPE_SYSLOG */
	else	/* LOG_TYPE_UNDEFINED == log_type */
	{
		LOCK_LOG;

		switch (level)
		{
			case LOG_LEVEL_CRIT:
				nt_error("ERROR: %s", message);
				break;
			case LOG_LEVEL_ERR:
				nt_error("Error: %s", message);
				break;
			case LOG_LEVEL_WARNING:
				nt_error("Warning: %s", message);
				break;
			case LOG_LEVEL_DEBUG:
				nt_error("DEBUG: %s", message);
				break;
			case LOG_LEVEL_TRACE:
				nt_error("TRACE: %s", message);
				break;
			default:
				nt_error("%s", message);
				break;
		}

		UNLOCK_LOG;
	}
}

int	nt_get_log_type(const char *logtype)
{
	const char	*logtypes[] = {NT_OPTION_LOGTYPE_SYSTEM, NT_OPTION_LOGTYPE_FILE, NT_OPTION_LOGTYPE_CONSOLE};
	int		i;

	for (i = 0; i < (int)ARRSIZE(logtypes); i++)
	{
		if (0 == strcmp(logtype, logtypes[i]))
			return i + 1;
	}

	return LOG_TYPE_UNDEFINED;
}

int	nt_validate_log_parameters(NT_TASK_EX *task)
{
	if (LOG_TYPE_UNDEFINED == CONFIG_LOG_TYPE)
	{
		nt_log(LOG_LEVEL_CRIT, "invalid \"LogType\" configuration parameter: '%s'", CONFIG_LOG_TYPE_STR);
		return FAIL;
	}

	if (LOG_TYPE_CONSOLE == CONFIG_LOG_TYPE && 0 == (task->flags & NT_TASK_FLAG_FOREGROUND) &&
			NT_TASK_START == task->task)
	{
		nt_log(LOG_LEVEL_CRIT, "\"LogType\" \"console\" parameter can only be used with the"
				" -f (--foreground) command line option");
		return FAIL;
	}

	if (LOG_TYPE_FILE == CONFIG_LOG_TYPE && (NULL == CONFIG_LOG_FILE || '\0' == *CONFIG_LOG_FILE))
	{
		nt_log(LOG_LEVEL_CRIT, "\"LogType\" \"file\" parameter requires \"LogFile\" parameter to be set");
		return FAIL;
	}

	return SUCCEED;
}

char	*nt_strerror(int errnum)
{
	/* !!! Attention: static !!! Not thread-safe for Win32 */
	static char	utf8_string[NT_MESSAGE_BUF_SIZE];

	nt_snprintf(utf8_string, sizeof(utf8_string), "[%d] %s", errnum, strerror(errnum));

	return utf8_string;
}

char	*strerror_from_system(unsigned long error)
{
	NT_UNUSED(error);
	return nt_strerror(errno);
}
