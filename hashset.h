#ifndef HASHSET_H
void* nt_hashset_insert_kv(nt_hashset_t *hs, const char* key, const void *val, size_t size);
void* nt_hashset_get_kv(nt_hashset_t *hs, const char* key);
#endif
