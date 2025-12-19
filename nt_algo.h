#ifndef NT_NTALGO_H
#define NT_NTALGO_H

#include "common.h"

/* generic */

typedef nt_uint32_t nt_hash_t;

nt_hash_t	nt_hash_lookup2(const void *data, size_t len, nt_hash_t seed);
nt_hash_t	nt_hash_modfnv(const void *data, size_t len, nt_hash_t seed);
nt_hash_t	nt_hash_murmur2(const void *data, size_t len, nt_hash_t seed);
nt_hash_t	nt_hash_sdbm(const void *data, size_t len, nt_hash_t seed);
nt_hash_t	nt_hash_djb2(const void *data, size_t len, nt_hash_t seed);

#define NT_DEFAULT_HASH_ALGO		nt_hash_modfnv
#define NT_DEFAULT_PTR_HASH_ALGO	nt_hash_modfnv
#define NT_DEFAULT_UINT64_HASH_ALGO	nt_hash_modfnv
#define NT_DEFAULT_STRING_HASH_ALGO	nt_hash_modfnv

typedef nt_hash_t (*nt_hash_func_t)(const void *data);

nt_hash_t	nt_default_ptr_hash_func(const void *data);
nt_hash_t	nt_default_uint64_hash_func(const void *data);
nt_hash_t	nt_default_string_hash_func(const void *data);
nt_hash_t	nt_default_uint64_pair_hash_func(const void *data);

#define NT_DEFAULT_HASH_SEED		0

#define NT_DEFAULT_PTR_HASH_FUNC		nt_default_ptr_hash_func
#define NT_DEFAULT_UINT64_HASH_FUNC		nt_default_uint64_hash_func
#define NT_DEFAULT_STRING_HASH_FUNC		nt_default_string_hash_func
#define NT_DEFAULT_UINT64_PAIR_HASH_FUNC	nt_default_uint64_pair_hash_func

typedef int (*nt_compare_func_t)(const void *d1, const void *d2);

int	nt_default_int_compare_func(const void *d1, const void *d2);
int	nt_default_uint64_compare_func(const void *d1, const void *d2);
int	nt_default_uint64_ptr_compare_func(const void *d1, const void *d2);
int	nt_default_str_compare_func(const void *d1, const void *d2);
int	nt_default_ptr_compare_func(const void *d1, const void *d2);
int	nt_default_uint64_pair_compare_func(const void *d1, const void *d2);

#define NT_DEFAULT_INT_COMPARE_FUNC		nt_default_int_compare_func
#define NT_DEFAULT_UINT64_COMPARE_FUNC		nt_default_uint64_compare_func
#define NT_DEFAULT_UINT64_PTR_COMPARE_FUNC	nt_default_uint64_ptr_compare_func
#define NT_DEFAULT_STR_COMPARE_FUNC		nt_default_str_compare_func
#define NT_DEFAULT_PTR_COMPARE_FUNC		nt_default_ptr_compare_func
#define NT_DEFAULT_UINT64_PAIR_COMPARE_FUNC	nt_default_uint64_pair_compare_func

typedef void *(*nt_mem_malloc_func_t)(void *old, size_t size);
typedef void *(*nt_mem_realloc_func_t)(void *old, size_t size);
typedef void (*nt_mem_free_func_t)(void *ptr);

void	*nt_default_mem_malloc_func(void *old, size_t size);
void	*nt_default_mem_realloc_func(void *old, size_t size);
void	nt_default_mem_free_func(void *ptr);

#define NT_DEFAULT_MEM_MALLOC_FUNC	nt_default_mem_malloc_func
#define NT_DEFAULT_MEM_REALLOC_FUNC	nt_default_mem_realloc_func
#define NT_DEFAULT_MEM_FREE_FUNC	nt_default_mem_free_func

typedef void (*nt_clean_func_t)(void *data);

#define NT_RETURN_IF_NOT_EQUAL(a, b)	\
					\
	if ((a) < (b))			\
		return -1;		\
	if ((a) > (b))			\
		return +1

int	is_prime(int n);
int	next_prime(int n);

/* pair */

typedef struct
{
	void	*first;
	void	*second;
}
nt_ptr_pair_t;

typedef struct
{
	nt_uint64_t	first;
	nt_uint64_t	second;
}
nt_uint64_pair_t;

/* hashset */

#define NT_HASHSET_ENTRY_T	struct nt_hashset_entry_s

NT_HASHSET_ENTRY_T
{
	NT_HASHSET_ENTRY_T	*next;
	nt_hash_t		hash;
#if SIZEOF_VOID_P > 4
	/* the data member must be properly aligned on 64-bit architectures that require aligned memory access */
	char			padding[sizeof(void *) - sizeof(nt_hash_t)];
#endif
	char			data[1];
};

typedef struct
{
	NT_HASHSET_ENTRY_T	**slots;
	int			num_slots;
	int			num_data;
	nt_hash_func_t		hash_func;
	nt_compare_func_t	compare_func;
	nt_clean_func_t	clean_func;
	nt_mem_malloc_func_t	mem_malloc_func;
	nt_mem_realloc_func_t	mem_realloc_func;
	nt_mem_free_func_t	mem_free_func;
} nt_hashset_t;

void	nt_hashset_create(nt_hashset_t *hs, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func);
void	nt_hashset_create_ext(nt_hashset_t *hs, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func,
				nt_clean_func_t clean_func,
				nt_mem_malloc_func_t mem_malloc_func,
				nt_mem_realloc_func_t mem_realloc_func,
				nt_mem_free_func_t mem_free_func);
void	nt_hashset_destroy(nt_hashset_t *hs);

void	*nt_hashset_insert(nt_hashset_t *hs, const void *data, size_t size);
void	*nt_hashset_insert_ext(nt_hashset_t *hs, const void *data, size_t size, size_t offset);
void	*nt_hashset_search(nt_hashset_t *hs, const void *data);
void	nt_hashset_remove(nt_hashset_t *hs, const void *data);
void	nt_hashset_remove_direct(nt_hashset_t *hs, const void *data);

void	nt_hashset_clear(nt_hashset_t *hs);

typedef struct
{
	nt_hashset_t		*hashset;
	int			slot;
	NT_HASHSET_ENTRY_T	*entry;
}
nt_hashset_iter_t;

void	nt_hashset_iter_reset(nt_hashset_t *hs, nt_hashset_iter_t *iter);
void	*nt_hashset_iter_next(nt_hashset_iter_t *iter);
void	nt_hashset_iter_remove(nt_hashset_iter_t *iter);

/* hashmap */

/* currently, we only have a very specialized hashmap */
/* that maps nt_uint64_t keys into non-negative ints */

#define NT_HASHMAP_ENTRY_T	struct nt_hashmap_entry_s
#define NT_HASHMAP_SLOT_T	struct nt_hashmap_slot_s

NT_HASHMAP_ENTRY_T
{
	nt_uint64_t	key;
//	int		value;
	nt_uint64_t	value;
};

NT_HASHMAP_SLOT_T
{
	NT_HASHMAP_ENTRY_T	*entries;
	int			entries_num;
	int			entries_alloc;
};

typedef struct
{
	NT_HASHMAP_SLOT_T	*slots;
	int			num_slots;
	int			num_data;
	nt_hash_func_t		hash_func;
	nt_compare_func_t	compare_func;
	nt_mem_malloc_func_t	mem_malloc_func;
	nt_mem_realloc_func_t	mem_realloc_func;
	nt_mem_free_func_t	mem_free_func;
}
nt_hashmap_t;

void	nt_hashmap_create(nt_hashmap_t *hm, size_t init_size);
void	nt_hashmap_create_ext(nt_hashmap_t *hm, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func,
				nt_mem_malloc_func_t mem_malloc_func,
				nt_mem_realloc_func_t mem_realloc_func,
				nt_mem_free_func_t mem_free_func);
void	nt_hashmap_destroy(nt_hashmap_t *hm);

//int	nt_hashmap_get(nt_hashmap_t *hm, nt_uint64_t key);
//void	nt_hashmap_set(nt_hashmap_t *hm, nt_uint64_t key, int value);
nt_uint64_t	nt_hashmap_get(nt_hashmap_t *hm, nt_uint64_t key);
void	nt_hashmap_set(nt_hashmap_t *hm, nt_uint64_t key, nt_uint64_t value);

nt_uint64_t	nt_hashmap_get_ext(nt_hashmap_t *hm, const char* key);
void	nt_hashmap_set_ext(nt_hashmap_t *hm, const char* key, nt_uint64_t value);

void	nt_hashmap_remove(nt_hashmap_t *hm, nt_uint64_t key);
void	nt_hashmap_clear(nt_hashmap_t *hm);

/* binary heap (min-heap) */

/* currently, we only have a very specialized binary heap that can */
/* store nt_uint64_t keys with arbitrary auxiliary information */

#define NT_BINARY_HEAP_OPTION_EMPTY	0
#define NT_BINARY_HEAP_OPTION_DIRECT	(1<<0)	/* support for direct update() and remove() operations */

typedef struct
{
	nt_uint64_t		key;
	const void		*data;
}
nt_binary_heap_elem_t;

typedef struct
{
	nt_binary_heap_elem_t	*elems;
	int			elems_num;
	int			elems_alloc;
	int			options;
	nt_compare_func_t	compare_func;
	nt_hashmap_t		*key_index;

	/* The binary heap is designed to work correctly only with memory allocation functions */
	/* that return pointer to the allocated memory or quit. Functions that can return NULL */
	/* are not supported (process will exit() if NULL return value is encountered). If     */
	/* using nt_mem_info_t and the associated memory functions then ensure that allow_oom */
	/* is always set to 0.                                                                 */
	nt_mem_malloc_func_t	mem_malloc_func;
	nt_mem_realloc_func_t	mem_realloc_func;
	nt_mem_free_func_t	mem_free_func;
} nt_binary_heap_t;

void			nt_binary_heap_create(nt_binary_heap_t *heap, nt_compare_func_t compare_func, int options);
void			nt_binary_heap_create_ext(nt_binary_heap_t *heap, nt_compare_func_t compare_func, int options,
							nt_mem_malloc_func_t mem_malloc_func,
							nt_mem_realloc_func_t mem_realloc_func,
							nt_mem_free_func_t mem_free_func);
void			nt_binary_heap_destroy(nt_binary_heap_t *heap);

int			nt_binary_heap_empty(nt_binary_heap_t *heap);
nt_binary_heap_elem_t	*nt_binary_heap_find_min(nt_binary_heap_t *heap);
void			nt_binary_heap_insert(nt_binary_heap_t *heap, nt_binary_heap_elem_t *elem);
void			nt_binary_heap_update_direct(nt_binary_heap_t *heap, nt_binary_heap_elem_t *elem);
void			nt_binary_heap_remove_min(nt_binary_heap_t *heap);
void			nt_binary_heap_remove_direct(nt_binary_heap_t *heap, nt_uint64_t key);

void			nt_binary_heap_clear(nt_binary_heap_t *heap);

/* vector */

#define NT_VECTOR_DECL(__id, __type)										\
														\
typedef struct													\
{														\
	__type			*values;									\
	int			values_num;									\
	int			values_alloc;									\
	nt_mem_malloc_func_t	mem_malloc_func;								\
	nt_mem_realloc_func_t	mem_realloc_func;								\
	nt_mem_free_func_t	mem_free_func;									\
}														\
nt_vector_ ## __id ## _t;											\
														\
void	nt_vector_ ## __id ## _create(nt_vector_ ## __id ## _t *vector);					\
void	nt_vector_ ## __id ## _create_ext(nt_vector_ ## __id ## _t *vector,					\
						nt_mem_malloc_func_t mem_malloc_func,				\
						nt_mem_realloc_func_t mem_realloc_func,			\
						nt_mem_free_func_t mem_free_func);				\
void	nt_vector_ ## __id ## _destroy(nt_vector_ ## __id ## _t *vector);					\
														\
void	nt_vector_ ## __id ## _append(nt_vector_ ## __id ## _t *vector, __type value);			\
void	nt_vector_ ## __id ## _append_ptr(nt_vector_ ## __id ## _t *vector, __type *value);			\
void	nt_vector_ ## __id ## _append_array(nt_vector_ ## __id ## _t *vector, __type const *values,		\
									int values_num);			\
void	nt_vector_ ## __id ## _remove_noorder(nt_vector_ ## __id ## _t *vector, int index);			\
void	nt_vector_ ## __id ## _remove(nt_vector_ ## __id ## _t *vector, int index);				\
														\
void	nt_vector_ ## __id ## _sort(nt_vector_ ## __id ## _t *vector, nt_compare_func_t compare_func);	\
void	nt_vector_ ## __id ## _uniq(nt_vector_ ## __id ## _t *vector, nt_compare_func_t compare_func);	\
														\
int	nt_vector_ ## __id ## _nearestindex(const nt_vector_ ## __id ## _t *vector, const __type value,	\
									nt_compare_func_t compare_func);	\
int	nt_vector_ ## __id ## _bsearch(const nt_vector_ ## __id ## _t *vector, const __type value,		\
									nt_compare_func_t compare_func);	\
int	nt_vector_ ## __id ## _lsearch(const nt_vector_ ## __id ## _t *vector, const __type value, int *index,\
									nt_compare_func_t compare_func);	\
int	nt_vector_ ## __id ## _search(const nt_vector_ ## __id ## _t *vector, const __type value,		\
									nt_compare_func_t compare_func);	\
void	nt_vector_ ## __id ## _setdiff(nt_vector_ ## __id ## _t *left, const nt_vector_ ## __id ## _t *right,\
									nt_compare_func_t compare_func);	\
														\
void	nt_vector_ ## __id ## _reserve(nt_vector_ ## __id ## _t *vector, size_t size);			\
void	nt_vector_ ## __id ## _clear(nt_vector_ ## __id ## _t *vector);

#define NT_PTR_VECTOR_DECL(__id, __type)									\
														\
NT_VECTOR_DECL(__id, __type);											\
														\
void	nt_vector_ ## __id ## _clear_ext(nt_vector_ ## __id ## _t *vector, nt_clean_func_t clean_func);

NT_VECTOR_DECL(uint64, nt_uint64_t);
NT_PTR_VECTOR_DECL(str, char *);
NT_PTR_VECTOR_DECL(ptr, void *);
NT_VECTOR_DECL(ptr_pair, nt_ptr_pair_t);
NT_VECTOR_DECL(uint64_pair, nt_uint64_pair_t);

/* this function is only for use with nt_vector_XXX_clear_ext() */
/* and only if the vector does not contain nested allocations */
void	nt_ptr_free(void *data);

/* 128 bit unsigned integer handling */
#define uset128(base, hi64, lo64)	(base)->hi = hi64; (base)->lo = lo64

void	uinc128_64(nt_uint128_t *base, nt_uint64_t value);
void	uinc128_128(nt_uint128_t *base, const nt_uint128_t *value);
void	udiv128_64(nt_uint128_t *result, const nt_uint128_t *base, nt_uint64_t value);
void	umul64_64(nt_uint128_t *result, nt_uint64_t value, nt_uint64_t factor);

unsigned int	nt_isqrt32(unsigned int value);

/* expression evaluation */

#define NT_UNKNOWN_STR		"NT_UNKNOWN"	/* textual representation of NT_UNKNOWN */
#define NT_UNKNOWN_STR_LEN	NT_CONST_STRLEN(NT_UNKNOWN_STR)

int	evaluate(double *value, const char *expression, char *error, size_t max_error_len,
		nt_vector_ptr_t *unknown_msgs);

/* forecasting */

#define NT_MATH_ERROR	-1.0

typedef enum
{
	FIT_LINEAR,
	FIT_POLYNOMIAL,
	FIT_EXPONENTIAL,
	FIT_LOGARITHMIC,
	FIT_POWER,
	FIT_INVALID
}
nt_fit_t;

typedef enum
{
	MODE_VALUE,
	MODE_MAX,
	MODE_MIN,
	MODE_DELTA,
	MODE_AVG,
	MODE_INVALID
}
nt_mode_t;

int	nt_fit_code(char *fit_str, nt_fit_t *fit, unsigned *k, char **error);
int	nt_mode_code(char *mode_str, nt_mode_t *mode, char **error);
double	nt_forecast(double *t, double *x, int n, double now, double time, nt_fit_t fit, unsigned k, nt_mode_t mode);
double	nt_timeleft(double *t, double *x, int n, double now, double threshold, nt_fit_t fit, unsigned k);


/* fifo queue of pointers */

typedef struct
{
	void	**values;
	int	alloc_num;
	int	head_pos;
	int	tail_pos;
}
nt_queue_ptr_t;

#define nt_queue_ptr_empty(queue)	((queue)->head_pos == (queue)->tail_pos ? SUCCEED : FAIL)

int	nt_queue_ptr_values_num(nt_queue_ptr_t *queue);
void	nt_queue_ptr_reserve(nt_queue_ptr_t *queue, int num);
void	nt_queue_ptr_compact(nt_queue_ptr_t *queue);
void	nt_queue_ptr_create(nt_queue_ptr_t *queue);
void	nt_queue_ptr_destroy(nt_queue_ptr_t *queue);
void	nt_queue_ptr_push(nt_queue_ptr_t *queue, void *value);
void	*nt_queue_ptr_pop(nt_queue_ptr_t *queue);
void	nt_queue_ptr_remove_value(nt_queue_ptr_t *queue, const void *value);



#endif
