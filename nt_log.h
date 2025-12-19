#ifndef NT_LOG_H
#define NT_LOG_H

#include "common.h"

#define LOG_LEVEL_EMPTY		0	/* printing nothing (if not LOG_LEVEL_INFORMATION set) */
#define LOG_LEVEL_CRIT		1
#define LOG_LEVEL_ERR		2
#define LOG_LEVEL_WARNING	3
#define LOG_LEVEL_DEBUG		4
#define LOG_LEVEL_TRACE		5

#define LOG_LEVEL_INFORMATION	127	/* printing in any case no matter what level set */

#define LOG_TYPE_UNDEFINED	0
#define LOG_TYPE_SYSTEM		1
#define LOG_TYPE_FILE		2
#define LOG_TYPE_CONSOLE	3

#define NT_OPTION_LOGTYPE_SYSTEM	"system"
#define NT_OPTION_LOGTYPE_FILE		"file"
#define NT_OPTION_LOGTYPE_CONSOLE	"console"

#define LOG_ENTRY_INTERVAL_DELAY	60	/* seconds */

extern int	nt_log_level;
#define NT_CHECK_LOG_LEVEL(level)			\
		((LOG_LEVEL_INFORMATION != (level) &&	\
		((level) > nt_log_level || LOG_LEVEL_EMPTY == (level))) ? FAIL : SUCCEED)

typedef enum
{
	ERR_Z3001 = 3001,
	ERR_Z3002,
	ERR_Z3003,
	ERR_Z3004,
	ERR_Z3005,
	ERR_Z3006,
	ERR_Z3007
}
nt_err_codes_t;
//#define HAVE___VA_ARGS__
#ifdef HAVE___VA_ARGS__
#	define NT_NT_LOG_CHECK
#	define nt_log(level, ...)									\
													\
	do												\
	{												\
		if (SUCCEED == NT_CHECK_LOG_LEVEL(level))						\
			__nt_nt_log(level, __VA_ARGS__);						\
	}												\
	while (0)
#else
#	define nt_log __nt_nt_log
#endif

int		nt_open_log(int type, int level, const char *filename, char **error);
void		__nt_nt_log(int level, const char *fmt, ...) __nt_attr_format_printf(2, 3);
void		nt_close_log(void);

int		nt_increase_log_level(void);
int		nt_decrease_log_level(void);
const char	*nt_get_log_level_string(void);

char		*nt_strerror(int errnum);
char		*strerror_from_system(unsigned long error);

int		nt_redirect_stdio(const char *filename);

void		nt_handle_log(void);

int		nt_get_log_type(const char *logtype);
int		nt_validate_log_parameters(NT_TASK_EX *task);

#endif
