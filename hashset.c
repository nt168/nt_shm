
#include <stddef.h>
#include "common.h"
#include "nt_log.h"
#include "nt_algo.h"

static void	__hashset_free_entry(nt_hashset_t *hs, NT_HASHSET_ENTRY_T *entry);

#define	CRIT_LOAD_FACTOR	4/5
#define	SLOT_GROWTH_FACTOR	3/2

#define NT_HASHSET_DEFAULT_SLOTS	10

/* private hashset functions */

static void	__hashset_free_entry(nt_hashset_t *hs, NT_HASHSET_ENTRY_T *entry)
{
	if (NULL != hs->clean_func)
		hs->clean_func(entry->data);

	hs->mem_free_func(entry);
}

static int	nt_hashset_init_slots(nt_hashset_t *hs, size_t init_size)
{
	hs->num_data = 0;

	if (0 < init_size)
	{
		hs->num_slots = next_prime(init_size);

		if (NULL == (hs->slots = hs->mem_malloc_func(NULL, hs->num_slots * sizeof(NT_HASHSET_ENTRY_T *))))
			return FAIL;

		memset(hs->slots, 0, hs->num_slots * sizeof(NT_HASHSET_ENTRY_T *));
	}
	else
	{
		hs->num_slots = 0;
		hs->slots = NULL;
	}

	return SUCCEED;
}

/* public hashset interface */

void	nt_hashset_create(nt_hashset_t *hs, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func)
{
	nt_hashset_create_ext(hs, init_size, hash_func, compare_func, NULL,
					NT_DEFAULT_MEM_MALLOC_FUNC,
					NT_DEFAULT_MEM_REALLOC_FUNC,
					NT_DEFAULT_MEM_FREE_FUNC);
}

void	nt_hashset_create_ext(nt_hashset_t *hs, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func,
				nt_clean_func_t clean_func,
				nt_mem_malloc_func_t mem_malloc_func,
				nt_mem_realloc_func_t mem_realloc_func,
				nt_mem_free_func_t mem_free_func)
{
	hs->hash_func = hash_func;
	hs->compare_func = compare_func;
	hs->clean_func = clean_func;
	hs->mem_malloc_func = mem_malloc_func;
	hs->mem_realloc_func = mem_realloc_func;
	hs->mem_free_func = mem_free_func;

	nt_hashset_init_slots(hs, init_size);
}

void	nt_hashset_destroy(nt_hashset_t *hs)
{
	int			i;
	NT_HASHSET_ENTRY_T	*entry, *next_entry;

	for (i = 0; i < hs->num_slots; i++)
	{
		entry = hs->slots[i];

		while (NULL != entry)
		{
			next_entry = entry->next;
			__hashset_free_entry(hs, entry);
			entry = next_entry;
		}
	}

	hs->num_data = 0;
	hs->num_slots = 0;

	if (NULL != hs->slots)
	{
		hs->mem_free_func(hs->slots);
		hs->slots = NULL;
	}

	hs->hash_func = NULL;
	hs->compare_func = NULL;
	hs->mem_malloc_func = NULL;
	hs->mem_realloc_func = NULL;
	hs->mem_free_func = NULL;
}

void	*nt_hashset_insert(nt_hashset_t *hs, const void *data, size_t size)
{
	return nt_hashset_insert_ext(hs, data, size, 0);
}

void	*nt_hashset_insert_ext(nt_hashset_t *hs, const void *data, size_t size, size_t offset)
{
	int			slot;
	nt_hash_t		hash;
	NT_HASHSET_ENTRY_T	*entry;

	if (0 == hs->num_slots && SUCCEED != nt_hashset_init_slots(hs, NT_HASHSET_DEFAULT_SLOTS))
		return NULL;

	hash = hs->hash_func(data);

	slot = hash % hs->num_slots;
	entry = hs->slots[slot];

	while (NULL != entry)
	{
		if (entry->hash == hash && hs->compare_func(entry->data, data) == 0)
			break;

		entry = entry->next;
	}

	if (NULL == entry)
	{
		if (hs->num_data + 1 >= hs->num_slots * CRIT_LOAD_FACTOR)
		{
			int			inc_slots, new_slot;
			void			*slots;
			NT_HASHSET_ENTRY_T	**prev_next, *curr_entry, *tmp;

			inc_slots = next_prime(hs->num_slots * SLOT_GROWTH_FACTOR);

			if (NULL == (slots = hs->mem_realloc_func(hs->slots, inc_slots * sizeof(NT_HASHSET_ENTRY_T *))))
				return NULL;

			hs->slots = slots;

			memset(hs->slots + hs->num_slots, 0, (inc_slots - hs->num_slots) * sizeof(NT_HASHSET_ENTRY_T *));

			for (slot = 0; slot < hs->num_slots; slot++)
			{
				prev_next = &hs->slots[slot];
				curr_entry = hs->slots[slot];

				while (NULL != curr_entry)
				{
					if (slot != (new_slot = curr_entry->hash % inc_slots))
					{
						tmp = curr_entry->next;
						curr_entry->next = hs->slots[new_slot];
						hs->slots[new_slot] = curr_entry;

						*prev_next = tmp;
						curr_entry = tmp;
					}
					else
					{
						prev_next = &curr_entry->next;
						curr_entry = curr_entry->next;
					}
				}
			}

			hs->num_slots = inc_slots;

			/* recalculate new slot */
			slot = hash % hs->num_slots;
		}

		if (NULL == (entry = hs->mem_malloc_func(NULL, offsetof(NT_HASHSET_ENTRY_T, data) + size)))
			return NULL;

		memcpy((char *)entry->data + offset, (const char *)data + offset, size - offset);
		entry->hash = hash;
		entry->next = hs->slots[slot];
		hs->slots[slot] = entry;
		hs->num_data++;
	}

	return entry->data;
}

void	*nt_hashset_search(nt_hashset_t *hs, const void *data)
{
	int			slot;
	nt_hash_t		hash;
	NT_HASHSET_ENTRY_T	*entry;

	if (0 == hs->num_slots)
		return NULL;

	hash = hs->hash_func(data);

	slot = hash % hs->num_slots;
	entry = hs->slots[slot];

	while (NULL != entry)
	{
		if (entry->hash == hash && hs->compare_func(entry->data, data) == 0)
			break;

		entry = entry->next;
	}

	return (NULL != entry ? entry->data : NULL);
}

/******************************************************************************
 *                                                                            *
 * Purpose: remove a hashset entry using comparison with the given data       *
 *                                                                            *
 ******************************************************************************/
void	nt_hashset_remove(nt_hashset_t *hs, const void *data)
{
	int			slot;
	nt_hash_t		hash;
	NT_HASHSET_ENTRY_T	*entry;

	if (0 == hs->num_slots)
		return;

	hash = hs->hash_func(data);

	slot = hash % hs->num_slots;
	entry = hs->slots[slot];

	if (NULL != entry)
	{
		if (entry->hash == hash && hs->compare_func(entry->data, data) == 0)
		{
			hs->slots[slot] = entry->next;
			__hashset_free_entry(hs, entry);
			hs->num_data--;
		}
		else
		{
			NT_HASHSET_ENTRY_T	*prev_entry;

			prev_entry = entry;
			entry = entry->next;

			while (NULL != entry)
			{
				if (entry->hash == hash && hs->compare_func(entry->data, data) == 0)
				{
					prev_entry->next = entry->next;
					__hashset_free_entry(hs, entry);
					hs->num_data--;
					break;
				}

				prev_entry = entry;
				entry = entry->next;
			}
		}
	}
}

/******************************************************************************
 *                                                                            *
 * Purpose: remove a hashset entry using a data pointer returned to the user  *
 *          by nt_hashset_insert[_ext]() and nt_hashset_search() functions  *
 *                                                                            *
 ******************************************************************************/
void	nt_hashset_remove_direct(nt_hashset_t *hs, const void *data)
{
	int			slot;
	NT_HASHSET_ENTRY_T	*data_entry, *iter_entry;

	if (0 == hs->num_slots)
		return;

	data_entry = (NT_HASHSET_ENTRY_T *)((const char *)data - offsetof(NT_HASHSET_ENTRY_T, data));

	slot = data_entry->hash % hs->num_slots;
	iter_entry = hs->slots[slot];

	if (NULL != iter_entry)
	{
		if (iter_entry == data_entry)
		{
			hs->slots[slot] = data_entry->next;
			__hashset_free_entry(hs, data_entry);
			hs->num_data--;
		}
		else
		{
			while (NULL != iter_entry->next)
			{
				if (iter_entry->next == data_entry)
				{
					iter_entry->next = data_entry->next;
					__hashset_free_entry(hs, data_entry);
					hs->num_data--;
					break;
				}

				iter_entry = iter_entry->next;
			}
		}
	}
}

void	nt_hashset_clear(nt_hashset_t *hs)
{
	int			slot;
	NT_HASHSET_ENTRY_T	*entry;

	for (slot = 0; slot < hs->num_slots; slot++)
	{
		while (NULL != hs->slots[slot])
		{
			entry = hs->slots[slot];
			hs->slots[slot] = entry->next;
			__hashset_free_entry(hs, entry);
		}
	}

	hs->num_data = 0;
}

#define	ITER_START	(-1)
#define	ITER_FINISH	(-2)

void	nt_hashset_iter_reset(nt_hashset_t *hs, nt_hashset_iter_t *iter)
{
	iter->hashset = hs;
	iter->slot = ITER_START;
}

void	*nt_hashset_iter_next(nt_hashset_iter_t *iter)
{
	if (ITER_FINISH == iter->slot)
		return NULL;

	if (ITER_START != iter->slot && NULL != iter->entry && NULL != iter->entry->next)
	{
		iter->entry = iter->entry->next;
		return iter->entry->data;
	}

	while (1)
	{
		iter->slot++;

		if (iter->slot == iter->hashset->num_slots)
		{
			iter->slot = ITER_FINISH;
			return NULL;
		}

		if (NULL != iter->hashset->slots[iter->slot])
		{
			iter->entry = iter->hashset->slots[iter->slot];
			return iter->entry->data;
		}
	}
}

void	nt_hashset_iter_remove(nt_hashset_iter_t *iter)
{
	if (ITER_START == iter->slot || ITER_FINISH == iter->slot || NULL == iter->entry)
	{
		nt_log(LOG_LEVEL_CRIT, "removing a hashset entry through a bad iterator");
		exit(EXIT_FAILURE);
	}

	if (iter->hashset->slots[iter->slot] == iter->entry)
	{
		iter->hashset->slots[iter->slot] = iter->entry->next;
		__hashset_free_entry(iter->hashset, iter->entry);
		iter->hashset->num_data--;

		iter->slot--;
		iter->entry = NULL;
	}
	else
	{
		NT_HASHSET_ENTRY_T	*prev_entry = iter->hashset->slots[iter->slot];

		while (prev_entry->next != iter->entry)
			prev_entry = prev_entry->next;

		prev_entry->next = iter->entry->next;
		__hashset_free_entry(iter->hashset, iter->entry);
		iter->hashset->num_data--;

		iter->entry = prev_entry;
	}
}
/* 定义键值对的哈希表条目结构 */
typedef struct {
    const char* key;
    void* val;
} nt_hashset_kv_t;

void	*nt_hashset_insert_inner_kv(nt_hashset_t *hs, const char *key, const void* data, size_t size, size_t offset)
{
	int			slot;
	nt_hash_t		hash;
	NT_HASHSET_ENTRY_T	*entry;

	if (0 == hs->num_slots && SUCCEED != nt_hashset_init_slots(hs, NT_HASHSET_DEFAULT_SLOTS))
		return NULL;

	hash = hs->hash_func(key);

	slot = hash % hs->num_slots;
	entry = hs->slots[slot];

	while (NULL != entry)
	{
		if (entry->hash == hash && hs->compare_func(entry->data, data) == 0)
			break;

		entry = entry->next;
	}

	if (NULL == entry)
	{
		if (hs->num_data + 1 >= hs->num_slots * CRIT_LOAD_FACTOR)
		{
			int			inc_slots, new_slot;
			void			*slots;
			NT_HASHSET_ENTRY_T	**prev_next, *curr_entry, *tmp;

			inc_slots = next_prime(hs->num_slots * SLOT_GROWTH_FACTOR);

			if (NULL == (slots = hs->mem_realloc_func(hs->slots, inc_slots * sizeof(NT_HASHSET_ENTRY_T *))))
				return NULL;

			hs->slots = slots;

			memset(hs->slots + hs->num_slots, 0, (inc_slots - hs->num_slots) * sizeof(NT_HASHSET_ENTRY_T *));

			for (slot = 0; slot < hs->num_slots; slot++)
			{
				prev_next = &hs->slots[slot];
				curr_entry = hs->slots[slot];

				while (NULL != curr_entry)
				{
					if (slot != (new_slot = curr_entry->hash % inc_slots))
					{
						tmp = curr_entry->next;
						curr_entry->next = hs->slots[new_slot];
						hs->slots[new_slot] = curr_entry;

						*prev_next = tmp;
						curr_entry = tmp;
					}
					else
					{
						prev_next = &curr_entry->next;
						curr_entry = curr_entry->next;
					}
				}
			}

			hs->num_slots = inc_slots;

			/* recalculate new slot */
			slot = hash % hs->num_slots;
		}

		if (NULL == (entry = hs->mem_malloc_func(NULL, offsetof(NT_HASHSET_ENTRY_T, data) + size)))
			return NULL;

		memcpy((char *)entry->data + offset, (const char *)data + offset, size - offset);
		entry->hash = hash;
		entry->next = hs->slots[slot];
		hs->slots[slot] = entry;
		hs->num_data++;
	}

	return entry->data;
}

void* nt_hashset_insert_kv(nt_hashset_t *hs, const char* key, const void *val, size_t size)
{
    nt_hashset_kv_t* kv_entry = NULL;
	kv_entry = (nt_hashset_kv_t*)malloc(sizeof(nt_hashset_kv_t));
    memset(kv_entry, 0, sizeof(nt_hashset_kv_t));
    kv_entry->key = strdup(key);

    kv_entry->val = malloc(size);//hs->mem_malloc_func(NULL, size);
    memset(kv_entry->val, 0, size);
    memcpy(kv_entry->val, val, size);
    if (!kv_entry->val) {
        return NULL;
    }
    memcpy(kv_entry->val, val, size);

    size_t key_len = strlen(key);
    void* inserted_data = nt_hashset_insert_ext(hs, kv_entry, key_len + size, offsetof(nt_hashset_kv_t, val));
//    void* inserted_data = nt_hashset_insert_inner_kv(hs, key, kv_entry, key_len + size, 0);
    if (!inserted_data) {
        hs->mem_free_func(kv_entry->val);
    }

    return inserted_data;
}

void* nt_hashset_get_kv(nt_hashset_t *hs, const char* key)
{
   nt_hashset_kv_t search_kv;
   memset(&search_kv, 0, sizeof(nt_hashset_kv_t));
   search_kv.key = strdup(key);

//    size_t key_len = strlen(key);
    nt_hashset_kv_t* found_kv = (nt_hashset_kv_t*)nt_hashset_search(hs, &search_kv);

    if (found_kv != NULL) {
        return found_kv->val;
    }

    return NULL;
}
