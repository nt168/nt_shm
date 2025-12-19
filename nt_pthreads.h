#ifndef NT_THREADS_H
#define NT_THREADS_H

#include "common.h"

int	nt_fork(void);
int	nt_child_fork(void);

typedef struct
{
	int		server_num;
	int		process_num;
	unsigned char	process_type;
	void		*args;
}nt_thread_args_t;

#define nt_thread_exit(status) \
		_exit((int)(status)); \
		return ((unsigned)(status))

extern unsigned char	program_type;
int	daemon_start(int allow_root, const char *user, unsigned int flags);

#define SIG_CHECK_PARAMS(sig, siginfo, context)											\
		if (NULL == siginfo)												\
			printf("received [signal:%d(%s)] with NULL siginfo", sig, get_signal_name(sig));	\
		if (NULL == context)												\
			printf("received [signal:%d(%s)] with NULL context", sig, get_signal_name(sig))


#define NT_THREAD_ERROR	-1

#define NT_THREAD_HANDLE	pid_t
#define NT_THREAD_HANDLE_NULL	0

#define NT_THREAD_WAIT_EXIT	1

#define NT_THREAD_ENTRY_POINTER(pointer_name) \
	unsigned (* pointer_name)(void *)

#define NT_THREAD_ENTRY(entry_name, arg_name)	\
	unsigned entry_name(void *arg_name)

/* Calling _exit() to terminate child process immediately is important. See NT-5732 for details. */
#define nt_thread_exit(status) \
	_exit((int)(status)); \
	return ((unsigned)(status))

#define nt_sleep(sec) sleep((sec))

#define nt_thread_kill(h) kill(h, SIGUSR2)
#define nt_thread_kill_fatal(h) kill(h, SIGHUP)

#if 0
typedef struct
{
	unsigned char	process_type;
//	press_type  process_type;
	int		server_num;
	int		process_num;
	uint32_t threads_num;
	uint32_t pressure_value;
	uint32_t duration;
	uint32_t bind_core;
//	void		*args;
	const char* exepath;
	const char* args;
	const char* process_description;
}nt_thread_args_t;
#endif

int	nt_thread_wait(NT_THREAD_HANDLE thread);
void nt_threads_wait(NT_THREAD_HANDLE *threads, const int *threads_flags, int threads_num, int ret);
NT_THREAD_HANDLE nt_thread_start(NT_THREAD_ENTRY_POINTER(handler), nt_thread_args_t *thread_args);
#endif	/* NT_THREADS_H */
