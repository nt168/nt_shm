#ifndef NT_MEM_H
#define NT_MEM_H

#include "common.h"
#include "nt_mutexs.h"

typedef struct
{
	void		**buckets;
	void		*lo_bound;
	void		*hi_bound;
	nt_uint64_t	free_size;
	nt_uint64_t	used_size;
	nt_uint64_t	orig_size;
	nt_uint64_t	total_size;
	int		shm_id;

	/* Continue execution in out of memory situation.                         */
	/* Normally allocator forces exit when it runs out of allocatable memory. */
	/* Set this flag to 1 to allow execution in out of memory situations.     */
	char		allow_oom;

	NT_MUTEX	mem_lock;
	const char	*mem_descr;
	const char	*mem_param;
} nt_mem_info_t;

int	nt_mem_create(nt_mem_info_t **info, nt_uint64_t size, const char *descr, const char *param, int allow_oom, char **error);
#define	nt_mem_malloc(info, old, size) __nt_mem_malloc(__FILE__, __LINE__, info, old, size)
#define	nt_mem_realloc(info, old, size) __nt_mem_realloc(__FILE__, __LINE__, info, old, size)
#define	nt_mem_free(info, ptr)				\
											\
do											\
{											\
	__nt_mem_free(__FILE__, __LINE__, info, ptr);	\
	ptr = NULL;								\
}											\
while (0)

void	*__nt_mem_malloc(const char *file, int line, nt_mem_info_t *info, const void *old, size_t size);
void	*__nt_mem_realloc(const char *file, int line, nt_mem_info_t *info, void *old, size_t size);
void	__nt_mem_free(const char *file, int line, nt_mem_info_t *info, void *ptr);

void	nt_mem_clear(nt_mem_info_t *info);

void	nt_mem_dump_stats(nt_mem_info_t *info);

size_t	nt_mem_required_size(int chunks_num, const char *descr, const char *param);

#define NT_MEM_FUNC1_DECL_MALLOC(__prefix)				\
static void	*__prefix ## _mem_malloc_func(void *old, size_t size)
#define NT_MEM_FUNC1_DECL_REALLOC(__prefix)				\
static void	*__prefix ## _mem_realloc_func(void *old, size_t size)
#define NT_MEM_FUNC1_DECL_FREE(__prefix)				\
static void	__prefix ## _mem_free_func(void *ptr)

#define NT_MEM_FUNC1_IMPL_MALLOC(__prefix, __info)			\
									\
static void	*__prefix ## _mem_malloc_func(void *old, size_t size)	\
{									\
	return nt_mem_malloc(__info, old, size);			\
}

#define NT_MEM_FUNC1_IMPL_REALLOC(__prefix, __info)			\
									\
static void	*__prefix ## _mem_realloc_func(void *old, size_t size)	\
{									\
	return nt_mem_realloc(__info, old, size);			\
}

#define NT_MEM_FUNC1_IMPL_FREE(__prefix, __info)			\
									\
static void	__prefix ## _mem_free_func(void *ptr)			\
{									\
	nt_mem_free(__info, ptr);					\
}

#define NT_MEM_FUNC_DECL(__prefix)					\
									\
NT_MEM_FUNC1_DECL_MALLOC(__prefix);					\
NT_MEM_FUNC1_DECL_REALLOC(__prefix);					\
NT_MEM_FUNC1_DECL_FREE(__prefix);

#define NT_MEM_FUNC_IMPL(__prefix, __info)				\
									\
NT_MEM_FUNC1_IMPL_MALLOC(__prefix, __info);				\
NT_MEM_FUNC1_IMPL_REALLOC(__prefix, __info);				\
NT_MEM_FUNC1_IMPL_FREE(__prefix, __info);

#endif
