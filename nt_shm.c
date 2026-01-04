#include "nt_log.h"
#include "nt_algo.h"
#include "nt_mem.h"
#include "nt_log.h"
#include "nt_algo.h"
#include "nt_mutexs.h"
#include "nt_socket.h"
#include "nt_pthreads.h"
#include "hashset.h"
#include <stdbool.h>

const char	*progname = NULL;
const char	syslog_app_name[] = "nt_base";
char *CONFIG_PID_FILE = "/tmp/pt_server.pid";

pthread_t tid;
int	CONFIG_LOG_LEVEL = LOG_LEVEL_WARNING;
extern char	*CONFIG_LOG_FILE;
extern int CONFIG_LOG_TYPE;

typedef struct envckd{
	char desc[20];
	bool stat;
}envckd;

//static nt_mem_info_t	*phy_mem = NULL;
static nt_mem_info_t	*trend_mem = NULL;
//static nt_mem_info_t    *phy_index_mem = NULL;

static nt_mem_info_t *sync_mem = NULL;
static nt_mem_info_t *sync_index_mem = NULL;
static nt_mem_info_t *mem = NULL;
static nt_mem_info_t *ntmap_mem = NULL;

nt_uint64_t	SERVER_UI_SYNC_CACHE_SIZE	= 16 * NT_MEBIBYTE;
NT_MEM_FUNC_IMPL(__trend, trend_mem);
NT_MEM_FUNC_IMPL(__sync_index, sync_index_mem);
NT_MEM_FUNC_IMPL(__sync, sync_mem);
NT_MEM_FUNC_IMPL(__ntmap, ntmap_mem);


static NT_MUTEX	ntmap_lock = NT_MUTEX_NULL;
static NT_MUTEX	sync_lock = NT_MUTEX_NULL;
static NT_MUTEX	trends_lock = NT_MUTEX_NULL;
static NT_MUTEX	cache_lock = NT_MUTEX_NULL;
static NT_MUTEX cache_ids_lock = NT_MUTEX_NULL;

#define	LOCK_CACHE	nt_mutex_lock(&cache_lock)
#define	UNLOCK_CACHE	nt_mutex_unlock(&cache_lock)

#define NT_PROGRAM_TYPE_SERVER		0x01
unsigned char	program_type		= NT_PROGRAM_TYPE_SERVER;

#	define NT_MUTEX_TRENDS		2
nt_uint64_t	CONFIG_TRENDS_CACHE_SIZE	= 4 * NT_MEBIBYTE;

typedef struct
{
	nt_uint64_t	history_counter;	/* the total number of processed values */
	nt_uint64_t	history_float_counter;	/* the number of processed float values */
	nt_uint64_t	history_uint_counter;	/* the number of processed uint values */
	nt_uint64_t	history_str_counter;	/* the number of processed str values */
	nt_uint64_t	history_log_counter;	/* the number of processed log values */
	nt_uint64_t	history_text_counter;	/* the number of processed text values */
	nt_uint64_t	notsupported_counter;	/* the number of processed not supported items */
}NT_DC_STATS;

typedef struct
{
	nt_hashset_t		trends;
	NT_DC_STATS		stats;

	nt_hashset_t		sync_items;
	nt_binary_heap_t	sync_queue;
	nt_hashmap_t    sync_hmap;

	int			history_num;
	int			trends_num;
	int			trends_last_cleanup_hour;
} NT_SYNC_CACHE;

static NT_SYNC_CACHE	*cache = NULL;

static NT_SYNC_CACHE	*ncache = NULL;

//void init_sync_cache(char** error)
//{
//	int ret;
//	if (SUCCEED != (ret = nt_mem_create(&sync_mem, SERVER_UI_SYNC_CACHE_SIZE, "sync cache",
//			"SyncCacheSize", 1, &error)))
//	{
//		goto out;
//	}
//
//	cache = (NT_DC_CACHE *)__hc_index_mem_malloc_func(NULL, sizeof(NT_DC_CACHE));
//	memset(cache, 0, sizeof(NT_DC_CACHE));
//out:
//	return;
//}

#	define NT_MUTEX_CACHE		1
#	define NT_MUTEX_CACHE_IDS	3
nt_uint64_t	CONFIG_SYNC_CACHE_SIZE	= 16 * NT_MEBIBYTE;
nt_uint64_t	CONFIG_SYNC_INDEX_CACHE_SIZE	= 4 * NT_MEBIBYTE;


/* flags */
#define NT_NOTNULL		0x01
#define NT_PROXY		0x02

/* FK flags */
#define NT_FK_CASCADE_DELETE	0x01

/* field types */
#define	NT_TYPE_INT		0
#define	NT_TYPE_CHAR		1
#define	NT_TYPE_FLOAT		2
#define	NT_TYPE_BLOB		3
#define	NT_TYPE_TEXT		4
#define	NT_TYPE_UINT		5
#define	NT_TYPE_ID		6
#define	NT_TYPE_SHORTTEXT	7
#define	NT_TYPE_LONGTEXT	8

#define NT_MAX_FIELDS		73 /* maximum number of fields in a table plus one for null terminator in dbschema.c */
#define NT_TABLENAME_LEN	26
#define NT_TABLENAME_LEN_MAX	(NT_TABLENAME_LEN + 1)
#define NT_FIELDNAME_LEN	28
#define NT_FIELDNAME_LEN_MAX	(NT_FIELDNAME_LEN + 1)
#define NT_IDS_SIZE	8

#define NT_SYNC_ITEMS_INIT_SIZE	1000

//typedef struct
//{
//	nt_hashset_t		trends;
//	NT_DC_STATS		stats;
//
//	nt_hashset_t		history_items;
//	nt_binary_heap_t	history_queue;
//
//	int			history_num;
//	int			trends_num;
//	int			trends_last_cleanup_hour;
//} NT_DC_CACHE;

typedef struct
{
	char		table_name[NT_TABLENAME_LEN_MAX];
	nt_uint64_t	lastid;
} NT_SYNC_ID;

typedef struct
{
	NT_SYNC_ID	id[NT_IDS_SIZE];
} NT_SYNC_IDS;


typedef struct
{
	int	sec;	/* seconds */
	int	ns;	/* nanoseconds */
} nt_timespec_t;

typedef struct
{
	int	timestamp;
	int	logeventid;
	int	severity;
	char	*source;
	char	*value;
} nt_log_value_t;

typedef union
{
	double		dbl;
	nt_uint64_t	ui64;
	char		*str;
	char		*err;
	nt_log_value_t	*log;
} history_value_t;

struct nt_sync_data
{
	history_value_t	value;
	nt_uint64_t	lastlogsize;
	nt_timespec_t	ts;
	int		mtime;
	unsigned char	value_type;
	unsigned char	flags;
	unsigned char	state;

	struct nt_sync_data	*next;
};

static NT_SYNC_IDS *ids = NULL;

typedef struct nt_sync_data	nt_sync_data_t;
typedef struct
{
	nt_uint64_t	itemid;
	unsigned char	status;

	nt_sync_data_t	*tail;
	nt_sync_data_t	*head;
} nt_sync_item_t;

#define nt_timespec_compare(t1, t2)	\
	((t1)->sec == (t2)->sec ? (t1)->ns - (t2)->ns : (t1)->sec - (t2)->sec)

static int	sync_queue_elem_compare_func(const void *d1, const void *d2)
{
	const nt_binary_heap_elem_t	*e1 = (const nt_binary_heap_elem_t *)d1;
	const nt_binary_heap_elem_t	*e2 = (const nt_binary_heap_elem_t *)d2;

	const nt_sync_item_t	*item1 = (const nt_sync_item_t *)e1->data;
	const nt_sync_item_t	*item2 = (const nt_sync_item_t *)e2->data;

	/* compare by timestamp of the oldest value */
	return nt_timespec_compare(&item1->tail->ts, &item2->tail->ts);
}

static int	init_trend_cache(char **error)
{
	const char	*__function_name = "init_trend_cache";
	size_t		sz;
	int		ret;

	nt_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (SUCCEED != (ret = nt_mutex_create(&trends_lock, NT_MUTEX_TRENDS, error)))
		goto out;

	sz = nt_mem_required_size(1, "trend cache", "TrendCacheSize");
	if (SUCCEED != (ret = nt_mem_create(&trend_mem, CONFIG_TRENDS_CACHE_SIZE, "trend cache", "TrendCacheSize", 0,
			error)))
	{
		goto out;
	}

	CONFIG_TRENDS_CACHE_SIZE -= sz;

	cache->trends_num = 0;
	cache->trends_last_cleanup_hour = 0;

#define INIT_HASHSET_SIZE	100	/* Should be calculated dynamically based on trends size? */
					/* Still does not make sense to have it more than initial */
					/* item hashset size in configuration cache.              */

	nt_hashset_create_ext(&cache->trends, INIT_HASHSET_SIZE,
			NT_DEFAULT_UINT64_HASH_FUNC, NT_DEFAULT_UINT64_COMPARE_FUNC, NULL,
			__trend_mem_malloc_func, __trend_mem_realloc_func, __trend_mem_free_func);

#undef INIT_HASHSET_SIZE
out:
	nt_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);

	return ret;
}

int	init_sync_cache(char **error)
{
	const char	*__function_name = "init_sync_cache";
	int		ret;
	nt_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (SUCCEED != (ret = nt_mutex_create(&cache_lock, NT_MUTEX_CACHE, error)))
		goto out;

//	if (SUCCEED != (ret = nt_mutex_create(&cache_ids_lock, NT_MUTEX_CACHE_IDS, error)))
//		goto out;

	if (SUCCEED != (ret = nt_mem_create(&sync_mem, CONFIG_SYNC_CACHE_SIZE, "sync cache",
			"SyncCacheSize", 1, error)))
	{
		goto out;
	}

	if (SUCCEED != (ret = nt_mem_create(&sync_index_mem, CONFIG_SYNC_INDEX_CACHE_SIZE, "sync index cache",
			"SyncIndexCacheSize", 0, error)))
	{
		goto out;
	}

	cache = (NT_SYNC_CACHE *)__sync_index_mem_malloc_func(NULL, sizeof(NT_SYNC_CACHE));
	memset(cache, 0, sizeof(NT_SYNC_CACHE));

	ids = (NT_SYNC_IDS *)__sync_index_mem_malloc_func(NULL, sizeof(NT_SYNC_IDS));
	memset(ids, 0, sizeof(NT_SYNC_IDS));

	nt_hashset_create_ext(&cache->sync_items, NT_SYNC_ITEMS_INIT_SIZE,
			NT_DEFAULT_UINT64_HASH_FUNC, NT_DEFAULT_UINT64_COMPARE_FUNC, NULL,
			__sync_index_mem_malloc_func, __sync_index_mem_realloc_func, __sync_index_mem_free_func);

	nt_binary_heap_create_ext(&cache->sync_queue, sync_queue_elem_compare_func, NT_BINARY_HEAP_OPTION_EMPTY,
			__sync_index_mem_malloc_func, __sync_index_mem_realloc_func, __sync_index_mem_free_func);

	nt_hashmap_create_ext(&cache->sync_hmap, 512,
			NT_DEFAULT_UINT64_HASH_FUNC,
			NT_DEFAULT_UINT64_COMPARE_FUNC,
			__sync_mem_malloc_func, __sync_mem_realloc_func, __sync_mem_free_func);

	if (0 != (program_type & NT_PROGRAM_TYPE_SERVER))
	{
		if (SUCCEED != (ret = init_trend_cache(error)))
			goto out;
	}

out:
	nt_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);

	return ret;
}

#	define NT_MUTEX_MAP		2
nt_uint64_t	CONFIG_NTMAP_CACHE_SIZE	= 4 * NT_MEBIBYTE;
static int init_nt_map(char **error)
{
	const char	*__function_name = "init_cache";
	size_t		sz;
	int		ret;

	nt_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (SUCCEED != (ret = nt_mutex_create(&ntmap_lock, NT_MUTEX_MAP, error)))
		goto out;

	sz = nt_mem_required_size(1, "map cache", "MapCacheSize");
	if (SUCCEED != (ret = nt_mem_create(&ntmap_mem, CONFIG_NTMAP_CACHE_SIZE, "map cache", "MapCacheSize", 0,
			error)))
	{
		goto out;
	}

	ncache = (NT_SYNC_CACHE *)__ntmap_mem_malloc_func(NULL, sizeof(NT_SYNC_CACHE));
		memset(ncache, 0, sizeof(NT_SYNC_CACHE));

	CONFIG_NTMAP_CACHE_SIZE -= sz;
	nt_hashmap_create_ext(&ncache->sync_hmap, 512,
				NT_DEFAULT_UINT64_HASH_FUNC,
				NT_DEFAULT_UINT64_COMPARE_FUNC,
				__ntmap_mem_malloc_func, __ntmap_mem_realloc_func, __ntmap_mem_free_func);
out:
	nt_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);

	return ret;
}

#	define NT_MUTEX_MEM		2
static int	init_cache(char **error)
{
	const char	*__function_name = "init_cache";
	size_t		sz;
	int		ret;

	nt_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (SUCCEED != (ret = nt_mutex_create(&sync_lock, NT_MUTEX_MEM, error)))
		goto out;

	sz = nt_mem_required_size(1, "trend cache", "TrendCacheSize");
	if (SUCCEED != (ret = nt_mem_create(&sync_mem, CONFIG_TRENDS_CACHE_SIZE, "trend cache", "TrendCacheSize", 0,
			error)))
	{
		goto out;
	}

	CONFIG_TRENDS_CACHE_SIZE -= sz;

	cache->trends_num = 0;
	cache->trends_last_cleanup_hour = 0;

#define INIT_HASHSET_SIZE	100
	nt_hashset_create_ext(&cache->trends, INIT_HASHSET_SIZE,
			NT_DEFAULT_UINT64_HASH_FUNC, NT_DEFAULT_UINT64_COMPARE_FUNC, NULL,
			__sync_mem_malloc_func, __sync_mem_realloc_func, __sync_mem_free_func);

#undef INIT_HASHSET_SIZE
out:
	nt_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);

	return ret;
}

void	free_sync_cache(void)
{
	const char	*__function_name = "free_sync_cache";

	nt_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

//	DCsync_all();

	cache = NULL;

	nt_mutex_destroy(&cache_lock);
	nt_mutex_destroy(&cache_ids_lock);

	if (0 != (program_type & NT_PROGRAM_TYPE_SERVER))
		nt_mutex_destroy(&trends_lock);

	nt_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/* item states */
#define ITEM_STATE_NORMAL		0
#define ITEM_STATE_NOTSUPPORTED		1

//#define NT_DC_FLAG_META	0x01	/* contains meta information (lastlogsize and mtime) */
#define NT_DC_FLAG_NOVALUE	0x02	/* entry contains no value */
//#define NT_DC_FLAG_LLD		0x04	/* low-level discovery value */
//#define NT_DC_FLAG_UNDEF	0x08	/* unsupported or undefined (delta calculation failed) value */
//#define NT_DC_FLAG_NOHISTORY	0x10	/* values should not be kept in history */
//#define NT_DC_FLAG_NOTRENDS	0x20	/* values should not be kept in trends */

/* item value types */
typedef enum
{
	ITEM_VALUE_TYPE_FLOAT = 0,
	ITEM_VALUE_TYPE_STR,
	ITEM_VALUE_TYPE_LOG,
	ITEM_VALUE_TYPE_UINT64,
	ITEM_VALUE_TYPE_TEXT,
	/* the number of defined value types */
	ITEM_VALUE_TYPE_MAX
} nt_item_value_type_t;

#if 0
static void	sync_free_data(nt_sync_data_t *data)
{
	if (ITEM_STATE_NOTSUPPORTED == data->state)
	{
		__sync_mem_free_func(data->value.str);
	}
	else
	{
		if (0 == (data->flags & NT_DC_FLAG_NOVALUE))
		{
			switch (data->value_type)
			{
				case ITEM_VALUE_TYPE_STR:
				case ITEM_VALUE_TYPE_TEXT:
					__sync_mem_free_func(data->value.str);
					break;
				case ITEM_VALUE_TYPE_LOG:
					__sync_mem_free_func(data->value.log->value);

					if (NULL != data->value.log->source)
						__sync_mem_free_func(data->value.log->source);

					__sync_mem_free_func(data->value.log);
					break;
			}
		}
	}
	__sync_mem_free_func(data);
}
#endif

#define QUEUE_NAME "/my_task_queue"
#define MAX_QUEUE_LENGTH 100
//#define THREAD_POOL_SIZE 4
#include "nt_cycle.h"
cycle_t *queue = NULL;

#define	LOCK_SYNC	nt_mutex_lock(&sync_lock)
#define	UNLOCK_SYNC	nt_mutex_unlock(&sync_lock)


int main()
{
	//	int ret = 0;
	cache = (NT_SYNC_CACHE*)malloc(sizeof(NT_SYNC_CACHE));
	memset(cache, 0, sizeof(NT_SYNC_CACHE));
	char *error = NULL;
#if 0

	if (SUCCEED != nt_locks_create(&error))
	{
		nt_error("cannot create locks: %s", error);
		nt_free(error);
		exit(EXIT_FAILURE);
	}

	if (SUCCEED != nt_open_log(CONFIG_LOG_TYPE, CONFIG_LOG_LEVEL, CONFIG_LOG_FILE, &error))
	{
		nt_error("cannot open log: %s", error);
		nt_free(error);
		exit(EXIT_FAILURE);
	}

	queue = task_queue_create(QUEUE_NAME, MAX_QUEUE_LENGTH);
    if (!queue) {
        fprintf(stderr, "Failed to create task queue\n");
        exit(EXIT_FAILURE);
    }

//    nt_socket(NULL);
    nt_thread_start(nt_socket, NULL);

    nt_thread_args_t *thread_args;
    thread_args = (nt_thread_args_t *)malloc(sizeof(nt_thread_args_t));
    memset(thread_args, 0, sizeof(nt_thread_args_t));
    thread_args->args = (void*)queue;
//    nt_thread_start(cycle_worker, thread_args);
    cycle_worker(thread_args);
//	nt_log(LOG_LEVEL_TRACE, "Starting Phy Server. Phy %s (revision %s).", "1.5", ".007");

	nt_log(LOG_LEVEL_WARNING, "Starting Phy Server. Phy %s (revision %s).", "1.5", ".007");
//	nt_mem_info_t *sync_mem = NULL;
//	nt_mem_malloc(&mem, );
#endif
#if 0

	init_nt_map(&error);
	init_cache(&error);
//	init_sync_cache(error);
	return 0;
#endif
	init_cache(&error);

	init_sync_cache(&error);
	char* p = NULL;
	p = (char*)malloc(128);
	memset(p, 0, 128);
	snprintf(p, 128, "%s", "phytune_v1.7.0");
	nt_hashset_insert_kv(&(cache->sync_items), "127.0.0.1", (void*)(p), 128);
	void* pp = nt_hashset_get_kv(&(cache->sync_items), "127.0.0.1");
	printf("%s\n", (char*)pp);	
//	nt_hashset_insert_kv(&(cache->sync_items), p, 128);
	nt_hashmap_set_ext(&(cache->sync_hmap), p, 1);
	nt_uint64_t ad = nt_hashmap_get_ext(&(cache->sync_hmap), p);
	printf("%lu\n", ad);
	char* ppp = (char*)ad;
	printf("%s\n", (char*)ppp);
//	printf("%d\n", *((int*)ppp));
	free((char*)ad);
	return 0;
}
