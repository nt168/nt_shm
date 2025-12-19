#include "common.h"
#include "nt_mutexs.h"
#include "nt_log.h"

#	if !HAVE_SEMUN
		union semun
		{
			int			val;	/* value for SETVAL */
			struct semid_ds		*buf;	/* buffer for IPC_STAT & IPC_SET */
			unsigned short int	*array;	/* array for GETALL & SETALL */
			struct seminfo		*__buf;	/* buffer for IPC_INFO */
		};

#		undef HAVE_SEMUN
#		define HAVE_SEMUN 1
#	endif	/* HAVE_SEMUN */

//#	include "cfg.h"
//#	include "ntthreads.h"

static int		NT_SEM_LIST_ID;
static unsigned char	mutexes;

int	nt_locks_create(char **error)
{
	union semun	semopts;
	int		i;

	if (-1 == (NT_SEM_LIST_ID = semget(IPC_PRIVATE, NT_MUTEX_COUNT + NT_RWLOCK_COUNT, 0600)))
	{
		*error = nt_dsprintf(*error, "cannot create semaphore set: %s", nt_strerror(errno));
		return FAIL;
	}

	/* set default semaphore value */

	semopts.val = 1;
	for (i = 0; NT_MUTEX_COUNT + NT_RWLOCK_COUNT > i; i++)
	{
		if (-1 != semctl(NT_SEM_LIST_ID, i, SETVAL, semopts))
			continue;

		*error = nt_dsprintf(*error, "cannot initialize semaphore: %s", nt_strerror(errno));

		if (-1 == semctl(NT_SEM_LIST_ID, 0, IPC_RMID, 0))
			nt_error("cannot remove semaphore set %d: %s", NT_SEM_LIST_ID, nt_strerror(errno));

		NT_SEM_LIST_ID = -1;

		return FAIL;
	}
	return SUCCEED;
}

nt_mutex_t	nt_mutex_addr_get(nt_mutex_name_t mutex_name)
{
	return mutex_name;
}

nt_rwlock_t	nt_rwlock_addr_get(nt_rwlock_name_t rwlock_name)
{
	return rwlock_name + NT_MUTEX_COUNT;
}

int	nt_rwlock_create(nt_rwlock_t *rwlock, nt_rwlock_name_t name, char **error)
{
	NT_UNUSED(error);
	*rwlock = name + NT_MUTEX_COUNT;
	mutexes++;
	return SUCCEED;
}

int	nt_mutex_create(nt_mutex_t *mutex, nt_mutex_name_t name, char **error)
{
	NT_UNUSED(error);
	mutexes++;
	*mutex = name;
	return SUCCEED;
}

void	__nt_mutex_lock(const char *filename, int line, nt_mutex_t mutex)
{

	struct sembuf	sem_lock;

	if (NT_MUTEX_NULL == mutex)
		return;

	sem_lock.sem_num = mutex;
	sem_lock.sem_op = -1;
	sem_lock.sem_flg = SEM_UNDO;

	while (-1 == semop(NT_SEM_LIST_ID, &sem_lock, 1))
	{
		if (EINTR != errno)
		{
			nt_error("[file:'%s',line:%d] lock failed: %s", filename, line, nt_strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

void	__nt_mutex_unlock(const char *filename, int line, nt_mutex_t mutex)
{
	struct sembuf	sem_unlock;

	if (NT_MUTEX_NULL == mutex)
		return;

	sem_unlock.sem_num = mutex;
	sem_unlock.sem_op = 1;
	sem_unlock.sem_flg = SEM_UNDO;

	while (-1 == semop(NT_SEM_LIST_ID, &sem_unlock, 1))
	{
		if (EINTR != errno)
		{
			nt_error("[file:'%s',line:%d] unlock failed: %s", filename, line, nt_strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

}

void	nt_mutex_destroy(nt_mutex_t *mutex)
{
	if (0 == --mutexes && -1 == semctl(NT_SEM_LIST_ID, 0, IPC_RMID, 0))
		nt_error("cannot remove semaphore set %d: %s", NT_SEM_LIST_ID, nt_strerror(errno));

	*mutex = NT_MUTEX_NULL;
}

