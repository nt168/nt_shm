
#include "common.h"
#include "nt_log.h"

#include "nt_algo.h"

static void	__hashmap_ensure_free_entry(nt_hashmap_t *hm, NT_HASHMAP_SLOT_T *slot);

#define	CRIT_LOAD_FACTOR	5/1
#define	SLOT_GROWTH_FACTOR	3/2

#define ARRAY_GROWTH_FACTOR	2 /* because the number of slot entries is usually small, with 3/2 they grow too slow */

#define NT_HASHMAP_DEFAULT_SLOTS	10

/* private hashmap functions */

static void	__hashmap_ensure_free_entry(nt_hashmap_t *hm, NT_HASHMAP_SLOT_T *slot)
{
	if (NULL == slot->entries)
	{
		slot->entries_num = 0;
		slot->entries_alloc = 6;
		slot->entries = hm->mem_malloc_func(NULL, slot->entries_alloc * sizeof(NT_HASHMAP_ENTRY_T));
	}
	else if (slot->entries_num == slot->entries_alloc)
	{
		slot->entries_alloc = slot->entries_alloc * ARRAY_GROWTH_FACTOR;
		slot->entries = hm->mem_realloc_func(slot->entries, slot->entries_alloc * sizeof(NT_HASHMAP_ENTRY_T));
	}
}

static void	nt_hashmap_init_slots(nt_hashmap_t *hm, size_t init_size)
{
	hm->num_data = 0;

	if (0 < init_size)
	{
		hm->num_slots = next_prime(init_size);
		hm->slots = hm->mem_malloc_func(NULL, hm->num_slots * sizeof(NT_HASHMAP_SLOT_T));
		memset(hm->slots, 0, hm->num_slots * sizeof(NT_HASHMAP_SLOT_T));
	}
	else
	{
		hm->num_slots = 0;
		hm->slots = NULL;
	}
}

/* public hashmap interface */

void	nt_hashmap_create(nt_hashmap_t *hm, size_t init_size)
{
	nt_hashmap_create_ext(hm, init_size,
				NT_DEFAULT_UINT64_HASH_FUNC,
				NT_DEFAULT_UINT64_COMPARE_FUNC,
				NT_DEFAULT_MEM_MALLOC_FUNC,
				NT_DEFAULT_MEM_REALLOC_FUNC,
				NT_DEFAULT_MEM_FREE_FUNC);
}

void	nt_hashmap_create_ext(nt_hashmap_t *hm, size_t init_size,
				nt_hash_func_t hash_func,
				nt_compare_func_t compare_func,
				nt_mem_malloc_func_t mem_malloc_func,
				nt_mem_realloc_func_t mem_realloc_func,
				nt_mem_free_func_t mem_free_func)
{
	hm->hash_func = hash_func;
	hm->compare_func = compare_func;
	hm->mem_malloc_func = mem_malloc_func;
	hm->mem_realloc_func = mem_realloc_func;
	hm->mem_free_func = mem_free_func;

	nt_hashmap_init_slots(hm, init_size);
}

void	nt_hashmap_destroy(nt_hashmap_t *hm)
{
	int	i;

	for (i = 0; i < hm->num_slots; i++)
	{
		if (NULL != hm->slots[i].entries)
			hm->mem_free_func(hm->slots[i].entries);
	}

	hm->num_data = 0;
	hm->num_slots = 0;

	if (NULL != hm->slots)
	{
		hm->mem_free_func(hm->slots);
		hm->slots = NULL;
	}

	hm->hash_func = NULL;
	hm->compare_func = NULL;
	hm->mem_malloc_func = NULL;
	hm->mem_realloc_func = NULL;
	hm->mem_free_func = NULL;
}
#if 0
int	nt_hashmap_get(nt_hashmap_t *hm, nt_uint64_t key)
{
	int			i, value = FAIL;
	nt_hash_t		hash;
	NT_HASHMAP_SLOT_T	*slot;

	if (0 == hm->num_slots)
		return FAIL;

	hash = hm->hash_func(&key);
	slot = &hm->slots[hash % hm->num_slots];

	for (i = 0; i < slot->entries_num; i++)
	{
		if (0 == hm->compare_func(&slot->entries[i].key, &key))
		{
			value = slot->entries[i].value;
			break;
		}
	}

	return value;
}
#endif

nt_uint64_t	nt_hashmap_get(nt_hashmap_t *hm, nt_uint64_t key)
{
	int			i;
	nt_uint64_t value = 0;
	nt_hash_t		hash;
	NT_HASHMAP_SLOT_T	*slot;

	if (0 == hm->num_slots)
		return 0;

	hash = hm->hash_func(&key);
	slot = &hm->slots[hash % hm->num_slots];

	for (i = 0; i < slot->entries_num; i++)
	{
		if (0 == hm->compare_func(&slot->entries[i].key, &key))
		{
			value = slot->entries[i].value;
			break;
		}
	}

	return value;
}

//void	nt_hashmap_set(nt_hashmap_t *hm, nt_uint64_t key, int value)
void	nt_hashmap_set(nt_hashmap_t *hm, nt_uint64_t key, nt_uint64_t value)
{
	int			i;
	nt_hash_t		hash;
	NT_HASHMAP_SLOT_T	*slot;

	if (0 == hm->num_slots)
		nt_hashmap_init_slots(hm, NT_HASHMAP_DEFAULT_SLOTS);

	hash = hm->hash_func(&key);
	slot = &hm->slots[hash % hm->num_slots];

	for (i = 0; i < slot->entries_num; i++)
	{
		if (0 == hm->compare_func(&slot->entries[i].key, &key))
		{
			slot->entries[i].value = value;
			break;
		}
	}

	if (i == slot->entries_num)
	{
		__hashmap_ensure_free_entry(hm, slot);
		slot->entries[i].key = key;
		slot->entries[i].value = value;
		slot->entries_num++;
		hm->num_data++;

		if (hm->num_data >= hm->num_slots * CRIT_LOAD_FACTOR)
		{
			int			inc_slots, s;
			NT_HASHMAP_SLOT_T	*new_slot;

			inc_slots = next_prime(hm->num_slots * SLOT_GROWTH_FACTOR);

			hm->slots = hm->mem_realloc_func(hm->slots, inc_slots * sizeof(NT_HASHMAP_SLOT_T));
			memset(hm->slots + hm->num_slots, 0, (inc_slots - hm->num_slots) * sizeof(NT_HASHMAP_SLOT_T));

			for (s = 0; s < hm->num_slots; s++)
			{
				slot = &hm->slots[s];

				for (i = 0; i < slot->entries_num; i++)
				{
					hash = hm->hash_func(&slot->entries[i].key);
					new_slot = &hm->slots[hash % inc_slots];

					if (slot != new_slot)
					{
						__hashmap_ensure_free_entry(hm, new_slot);
						new_slot->entries[new_slot->entries_num] = slot->entries[i];
						new_slot->entries_num++;

						slot->entries[i] = slot->entries[slot->entries_num - 1];
						slot->entries_num--;
						i--;
					}
				}
			}

			hm->num_slots = inc_slots;
		}
	}
}

void	nt_hashmap_remove(nt_hashmap_t *hm, nt_uint64_t key)
{
	int			i;
	nt_hash_t		hash;
	NT_HASHMAP_SLOT_T	*slot;

	if (0 == hm->num_slots)
		return;

	hash = hm->hash_func(&key);
	slot = &hm->slots[hash % hm->num_slots];

	for (i = 0; i < slot->entries_num; i++)
	{
		if (0 == hm->compare_func(&slot->entries[i].key, &key))
		{
			slot->entries[i] = slot->entries[slot->entries_num - 1];
			slot->entries_num--;
			hm->num_data--;
			break;
		}
	}
}

void	nt_hashmap_clear(nt_hashmap_t *hm)
{
	int			i;
	NT_HASHMAP_SLOT_T	*slot;

	for (i = 0; i < hm->num_slots; i++)
	{
		slot = &hm->slots[i];

		if (NULL != slot->entries)
		{
			hm->mem_free_func(slot->entries);
			slot->entries = NULL;
			slot->entries_num = 0;
			slot->entries_alloc = 0;
		}
	}
	hm->num_data = 0;
}

nt_uint64_t MurmurHash64A(const void *key, int len, nt_uint64_t seed)
{
    const nt_uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    nt_uint64_t h = seed ^ (len * m);

    const nt_uint64_t *data = (const nt_uint64_t *)key;
    const nt_uint64_t *end = data + (len / 8);

    while(data != end)
    {
        nt_uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char*)data;

    switch(len & 7)
    {
        case 7: h ^= (nt_uint64_t)(data2[6]) << 48;
        case 6: h ^= (nt_uint64_t)(data2[5]) << 40;
        case 5: h ^= (nt_uint64_t)(data2[4]) << 32;
        case 4: h ^= (nt_uint64_t)(data2[3]) << 24;
        case 3: h ^= (nt_uint64_t)(data2[2]) << 16;
        case 2: h ^= (nt_uint64_t)(data2[1]) << 8;
        case 1: h ^= (nt_uint64_t)(data2[0]);
                h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

nt_uint64_t string_to_uint64(const char *str)
{
    nt_uint64_t seed = 0x12345678;
    return MurmurHash64A(str, strlen(str), seed);
}

nt_uint64_t	nt_hashmap_get_ext(nt_hashmap_t *hm, const char* key)
{
	nt_uint64_t ntkey;
	nt_uint64_t rtval;
	ntkey = string_to_uint64(key);
	rtval = nt_hashmap_get(hm, ntkey);
	return rtval;
}

void nt_hashmap_set_ext(nt_hashmap_t *hm, const char* key, nt_uint64_t value)
{
	nt_uint64_t ntkey;
	ntkey = string_to_uint64(key);
	nt_hashmap_set(hm, ntkey, value);
}
