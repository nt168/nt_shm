#include "nt_pthreads.h"
#include "common.h"
#include "nt_log.h"

static int	parent_pid = -1;
static FILE	*fpid = NULL;
static int	fdpid = -1;
int	sig_parent_pid = -1;
int	sig_exiting = 0;
extern char	*CONFIG_LOG_FILE;
extern int	CONFIG_LOG_TYPE;
const char	*get_signal_name(int sig);

extern char *CONFIG_PID_FILE;


#define SIG_PARENT_PROCESS		(sig_parent_pid == (int)getpid())

#define SIG_CHECKED_FIELD(siginfo, field)		(NULL == siginfo ? -1 : (int)siginfo->field)
#define SIG_CHECKED_FIELD_TYPE(siginfo, field, type)	(NULL == siginfo ? (type)-1 : siginfo->field)
#define SIG_PARENT_PROCESS				(sig_parent_pid == (int)getpid())
////////////////////////////////////////////////////
#define NT_THREAD_HANDLE	pid_t
#define NT_THREAD_HANDLE_NULL	0
#define NT_THREAD_ERROR	-1

NT_THREAD_LOCAL static char		*my_psk			= NULL;
NT_THREAD_LOCAL static size_t		my_psk_len		= 0;

int	nt_fork()
{
	fflush(stdout);
	fflush(stderr);
	return fork();
}

int	nt_child_fork()
{
	pid_t		pid;
	sigset_t	mask, orig_mask;

	/* block SIGTERM, SIGINT and SIGCHLD during fork to avoid deadlock (we've seen one in __unregister_atfork()) */
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, &orig_mask);

	pid = nt_fork();

	sigprocmask(SIG_SETMASK, &orig_mask, NULL);

	/* ignore SIGCHLD to avoid problems with exiting scripts in nt_execute() and other cases */
	if (0 == pid)
		signal(SIGCHLD, SIG_DFL);

	return pid;
}

NT_THREAD_HANDLE nt_thread_start(NT_THREAD_ENTRY_POINTER(handler), nt_thread_args_t *thread_args)
{
	NT_THREAD_HANDLE	thread = NT_THREAD_HANDLE_NULL;
	if (0 == (thread = nt_child_fork()))	/* child process */
	{
		(*handler)(thread_args);
		nt_thread_exit(EXIT_SUCCESS);
	}
	else if (-1 == thread)
	{
		nt_error("failed to fork: %s", nt_strerror(errno));
		thread = (NT_THREAD_HANDLE)NT_THREAD_ERROR;
	}
	return thread;
}

int	MAIN_SERVER_ENTRY(int flags)
{
//	nt_thread_start(nt_net_server, NULL);
//	nt_thread_start(messagechannel, NULL);
//	nt_thread_start(agents_synchronizer, NULL);

	return 0;
}


void nt_tls_free_on_signal(void)
{
	if (NULL != my_psk)
		nt_guaranteed_memset(my_psk, 0, my_psk_len);
}

int	create_pid_file(const char *pidfile)
{
	int		fd;
	nt_stat_t	buf;
	struct flock	fl;

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = getpid();

	/* check if pid file already exists */
	if (0 == nt_stat(pidfile, &buf))
	{
		if (-1 == (fd = open(pidfile, O_WRONLY | O_APPEND)))
		{
			nt_error("cannot open PID file [%s]: %s", pidfile, nt_strerror(errno));
			return FAIL;
		}

		if (-1 == fcntl(fd, F_SETLK, &fl))
		{
			close(fd);
			nt_error("Is this process already running? Could not lock PID file [%s]: %s",
					pidfile, nt_strerror(errno));
			return FAIL;
		}

		close(fd);
	}

	/* open pid file */
	if (NULL == (fpid = fopen(pidfile, "w")))
	{
		nt_error("cannot create PID file [%s]: %s", pidfile, nt_strerror(errno));
		return FAIL;
	}

	/* lock file */
	if (-1 != (fdpid = fileno(fpid)))
	{
		fcntl(fdpid, F_SETLK, &fl);
		fcntl(fdpid, F_SETFD, FD_CLOEXEC);
	}

	/* write pid to file */
	fprintf(fpid, "%d", (int)getpid());
	fflush(fpid);

	return SUCCEED;
}

void	exit_with_failure(void)
{
#if defined(HAVE_POLARSSL) || defined(HAVE_GNUTLS) || defined(HAVE_OPENSSL)
	nt_tls_free_on_signal();
#endif
	exit(EXIT_FAILURE);
}
#if 0
const char	*get_signal_name(int sig)
{
	switch (sig)
	{
		case SIGALRM:	return "SIGALRM";
		case SIGILL:	return "SIGILL";
		case SIGFPE:	return "SIGFPE";
		case SIGSEGV:	return "SIGSEGV";
		case SIGBUS:	return "SIGBUS";
		case SIGQUIT:	return "SIGQUIT";
		case SIGINT:	return "SIGINT";
		case SIGTERM:	return "SIGTERM";
		case SIGPIPE:	return "SIGPIPE";
		case SIGUSR1:	return "SIGUSR1";
		default:	return "unknown";
	}
}
#endif

void	child_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	if (!SIG_PARENT_PROCESS)
		exit_with_failure();

	if (0 == sig_exiting)
	{
		sig_exiting = 1;
//		unbench_log(LOG_LEVEL_CRIT, "One child process died (PID:%d,exitcode/signal:%d). Exiting ...",
//				SIG_CHECKED_FIELD(siginfo, si_pid), SIG_CHECKED_FIELD(siginfo, si_status));

#if defined(HAVE_POLARSSL) || defined(HAVE_GNUTLS) || defined(HAVE_OPENSSL)
		nt_tls_free_on_signal();
#endif
		nt_on_exit();
	}
}

void	drop_pid_file(const char *pidfile)
{
	struct flock	fl;

	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = nt_get_thread_id();

	/* unlock file */
	if (-1 != fdpid)
		fcntl(fdpid, F_SETLK, &fl);

	/* close pid file */
	nt_fclose(fpid);

	unlink(pidfile);
}

void	daemon_stop(void)
{
	/* this function is registered using atexit() to be called when we terminate */
	/* there should be nothing like logging or calls to exit() beyond this point */

	if (parent_pid != (int)getpid())
		return;
	drop_pid_file(CONFIG_PID_FILE);
}

void 	nt_set_child_signal_handler(void)
{
	struct sigaction	phan;
	sig_parent_pid = (int)getpid();

	sigemptyset(&phan.sa_mask);
	phan.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;

	phan.sa_sigaction = child_signal_handler;
	sigaction(SIGCHLD, &phan, NULL);
}

void pipe_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	printf("Got signal [signal:%d(%s),sender_pid:%d]. Ignoring ...",
			sig, get_signal_name(sig),
			SIG_CHECKED_FIELD(siginfo, si_pid));
}

void	user1_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	printf("Got signal [signal:%d(%s),sender_pid:%d,sender_uid:%d,value_int:%d(0x%08x)].",
			sig, get_signal_name(sig),
			SIG_CHECKED_FIELD(siginfo, si_pid),
			SIG_CHECKED_FIELD(siginfo, si_uid),
			SIG_CHECKED_FIELD(siginfo, si_value.sival_int),
			SIG_CHECKED_FIELD(siginfo, si_value.sival_int));
}

void	terminate_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	if (!SIG_PARENT_PROCESS)
	{
//		printf(sig_parent_pid == SIG_CHECKED_FIELD(siginfo, si_pid) || SIGINT == sig ? 4 : 3,
		nt_log(sig_parent_pid == SIG_CHECKED_FIELD(siginfo, si_pid) || SIGINT == sig ? 4 : 3,
				"Got signal [signal:%d(%s),sender_pid:%d,sender_uid:%d,"
				"reason:%d]. %s ...",
				sig, get_signal_name(sig),
				SIG_CHECKED_FIELD(siginfo, si_pid),
				SIG_CHECKED_FIELD(siginfo, si_uid),
				SIG_CHECKED_FIELD(siginfo, si_code),
				SIGINT == sig ? "Ignoring" : "Exiting");

		/* ignore interrupt signal in children - the parent */
		/* process will send terminate signals instead      */
		if (SIGINT == sig)
			return;
		exit_with_failure();
	}
	else
	{
		if (0 == sig_exiting)
		{
			sig_exiting = 1;
//			printf(sig_parent_pid == SIG_CHECKED_FIELD(siginfo, si_pid) ? LEVEL_DEBUG : LEVEL_WARNING,
			nt_log(sig_parent_pid == SIG_CHECKED_FIELD(siginfo, si_pid) ? LEVEL_DEBUG : LEVEL_WARNING,
					"Got signal [signal:%d(%s),sender_pid:%d,sender_uid:%d,"
					"reason:%d]. Exiting ...",
					sig, get_signal_name(sig),
					SIG_CHECKED_FIELD(siginfo, si_pid),
					SIG_CHECKED_FIELD(siginfo, si_uid),
					SIG_CHECKED_FIELD(siginfo, si_code));

#if defined(HAVE_POLARSSL) || defined(HAVE_GNUTLS) || defined(HAVE_OPENSSL)
			nt_tls_free_on_signal();
#endif
			nt_on_exit();
		}
	}
}

void log_fatal_signal(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	printf("Got signal [signal:%d(%s),reason:%d,refaddr:%p]. Crashing ...", \
			sig, get_signal_name(sig), \
			SIG_CHECKED_FIELD(siginfo, si_code), \
			SIG_CHECKED_FIELD_TYPE(siginfo, si_addr, void *));
}

void fatal_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
//	log_fatal_signal(sig, siginfo, context);
//	nt_log_fatal_info(context, NT_FATAL_LOG_FULL_INFO);
	exit_with_failure();
}

void	alarm_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	SIG_CHECK_PARAMS(sig, siginfo, context);

	nt_alarm_flag_set();	/* set alarm flag */
}

void	nt_set_common_signal_handlers(void)
{
	struct sigaction	phan;

	sig_parent_pid = (int)getpid();

	sigemptyset(&phan.sa_mask);
	phan.sa_flags = SA_SIGINFO;

	phan.sa_sigaction = terminate_signal_handler;
	sigaction(SIGINT, &phan, NULL);
	sigaction(SIGQUIT, &phan, NULL);
	sigaction(SIGTERM, &phan, NULL);

	phan.sa_sigaction = fatal_signal_handler;
	sigaction(SIGILL, &phan, NULL);
	sigaction(SIGFPE, &phan, NULL);
	sigaction(SIGSEGV, &phan, NULL);
	sigaction(SIGBUS, &phan, NULL);

	phan.sa_sigaction = alarm_signal_handler;
	sigaction(SIGALRM, &phan, NULL);
}

void	set_daemon_signal_handlers(void)
{
	struct sigaction	phan;

	sig_parent_pid = (int)getpid();

	sigemptyset(&phan.sa_mask);
	phan.sa_flags = SA_SIGINFO;

	phan.sa_sigaction = user1_signal_handler;
	sigaction(SIGUSR1, &phan, NULL);

	phan.sa_sigaction = pipe_signal_handler;
	sigaction(SIGPIPE, &phan, NULL);
}

int	daemon_start(int allow_root, const char *user, unsigned int flags)
{
	pid_t		pid;
	struct passwd	*pwd;

	if (0 == allow_root && 0 == getuid())	/* running as root? */
	{
		if (0 != (flags & NT_TASK_FLAG_FOREGROUND))
		{
			nt_error("cannot run as root!");
			exit(EXIT_FAILURE);
		}

		if (NULL == user)
			user = "root";

		pwd = getpwnam(user);

		if (NULL == pwd)
		{
			nt_error("user %s does not exist", user);
			nt_error("cannot run as root!");
			exit(EXIT_FAILURE);
		}

		if (0 == pwd->pw_uid)
		{
			nt_error("User=%s contradicts AllowRoot=0", user);
			nt_error("cannot run as root!");
			exit(EXIT_FAILURE);
		}

		if (-1 == setgid(pwd->pw_gid))
		{
			nt_error("cannot setgid to %s: %s", user, nt_strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (-1 == setuid(pwd->pw_uid))
		{
			nt_error("cannot setuid to %s: %s", user, nt_strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	umask(0002);

	if (0 == (flags & NT_TASK_FLAG_FOREGROUND))
	{
		if (0 != (pid = nt_fork()))
			exit(EXIT_SUCCESS);

		setsid();

		signal(SIGHUP, SIG_IGN);

		if (0 != (pid = nt_fork()))
			exit(EXIT_SUCCESS);

		if (-1 == chdir("/"))	/* this is to eliminate warning: ignoring return value of chdir */
			assert(0);

		nt_redirect_stdio(LOG_TYPE_FILE == CONFIG_LOG_TYPE ? CONFIG_LOG_FILE : NULL);
	}

	if (FAIL == create_pid_file(CONFIG_PID_FILE))
		exit(EXIT_FAILURE);

	atexit(daemon_stop);

	parent_pid = (int)getpid();

#if 1
	nt_set_common_signal_handlers();
	set_daemon_signal_handlers();
	nt_set_child_signal_handler();
	return MAIN_SERVER_ENTRY(flags);
#endif
}
