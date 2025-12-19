#ifndef NT_MUTEXS_H
#define NT_MUTEXS_H

#include <semaphore.h>

//#define HAVE_PTHREAD_PROCESS_SHARED

#ifdef _WINDOWS
#	define NT_MUTEX_NULL		NULL

#	define NT_MUTEX_LOG		nt_mutex_create_per_process_name(L"NT_MUTEX_LOG")
#	define NT_MUTEX_PERFSTAT	nt_mutex_create_per_process_name(L"NT_MUTEX_PERFSTAT")

typedef wchar_t * nt_mutex_name_t;
typedef HANDLE nt_mutex_t;
#else	/* not _WINDOWS */

#	define NT_MUTEX		int

typedef enum
{
	NT_MUTEX_LOG = 0,
	NT_MUTEX_COUNT
}
nt_mutex_name_t;

typedef enum
{
	NT_RWLOCK_CONFIG = 0,
	NT_RWLOCK_VALUECACHE,
	NT_RWLOCK_COUNT,
}
nt_rwlock_name_t;

#ifdef HAVE_PTHREAD_PROCESS_SHARED
#	define NT_MUTEX_NULL			NULL
#	define NT_RWLOCK_NULL			NULL

#	define nt_rwlock_wrlock(rwlock)	__nt_rwlock_wrlock(__FILE__, __LINE__, rwlock)
#	define nt_rwlock_rdlock(rwlock)	__nt_rwlock_rdlock(__FILE__, __LINE__, rwlock)
#	define nt_rwlock_unlock(rwlock)	__nt_rwlock_unlock(__FILE__, __LINE__, rwlock)

typedef pthread_mutex_t * nt_mutex_t;
typedef pthread_rwlock_t * nt_rwlock_t;

void	__nt_rwlock_wrlock(const char *filename, int line, nt_rwlock_t rwlock);
void	__nt_rwlock_rdlock(const char *filename, int line, nt_rwlock_t rwlock);
void	__nt_rwlock_unlock(const char *filename, int line, nt_rwlock_t rwlock);
void	nt_rwlock_destroy(nt_rwlock_t *rwlock);
void	nt_locks_disable(void);
#else	/* fallback to semaphores if read-write locks are not available */
#	define NT_RWLOCK_NULL				-1
#	define NT_MUTEX_NULL				-1

#	define nt_rwlock_wrlock(rwlock)		__nt_mutex_lock(__FILE__, __LINE__, rwlock)
#	define nt_rwlock_rdlock(rwlock)		__nt_mutex_lock(__FILE__, __LINE__, rwlock)
#	define nt_rwlock_unlock(rwlock)		__nt_mutex_unlock(__FILE__, __LINE__, rwlock)
#	define nt_rwlock_destroy(rwlock)		nt_mutex_destroy(rwlock)

typedef int nt_mutex_t;
typedef int nt_rwlock_t;
#endif
int		nt_locks_create(char **error);
int		nt_rwlock_create(nt_rwlock_t *rwlock, nt_rwlock_name_t name, char **error);
nt_mutex_t	nt_mutex_addr_get(nt_mutex_name_t mutex_name);
nt_rwlock_t	nt_rwlock_addr_get(nt_rwlock_name_t rwlock_name);
#endif	/* _WINDOWS */
#	define nt_mutex_lock(mutex)		__nt_mutex_lock(__FILE__, __LINE__, mutex)
#	define nt_mutex_unlock(mutex)		__nt_mutex_unlock(__FILE__, __LINE__, mutex)

int	nt_mutex_create(nt_mutex_t *mutex, nt_mutex_name_t name, char **error);
void	__nt_mutex_lock(const char *filename, int line, nt_mutex_t mutex);
void	__nt_mutex_unlock(const char *filename, int line, nt_mutex_t mutex);
void	nt_mutex_destroy(nt_mutex_t *mutex);

#ifdef _WINDOWS
nt_mutex_name_t	nt_mutex_create_per_process_name(const nt_mutex_name_t prefix);
#endif

#endif	/* NT_MUTEXS_H */
