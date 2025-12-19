#ifndef NT_FATAL_H
#define NT_FATAL_H

#include <signal.h>
#define HAVE_SIGNAL_H
//#define NT_GET_PC
#define NT_FATAL_LOG_PC_REG_SF		0x0001
#define NT_FATAL_LOG_BACKTRACE		0x0002
#define NT_FATAL_LOG_MEM_MAP		0x0004
#define NT_FATAL_LOG_FULL_INFO		(NT_FATAL_LOG_PC_REG_SF | NT_FATAL_LOG_BACKTRACE | NT_FATAL_LOG_MEM_MAP)

const char	*get_signal_name(int sig);
void	nt_log_fatal_info(void *context, unsigned int flags);

#endif
