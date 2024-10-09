#include "chashmap.h"
#include "cutils.h"
#include <assert.h>
#include <string.h>

#define HASHMAP_HASH_INIT 2166136261u
#define PSL               1
#define SWAP_CAP          2 // swap key value 的容量，不只是用于交换
#define SWAP_LEN          2 // 交换使用的长度
#define PTR_LEN           sizeof(uintptr_t)
#define NULL_KEY_HASH     0
#define NULL_KEY_PSL      SIZE_MAX

static inline void *mem_get_val(u8 *mem, usize vsize, usize index)
{
    return mem + (vsize * index);
}

/**
 * @brief FNV-1a hash
 *
 * @param data 数据
 * @param dsize 数据大小
 * @param seed 随机种子
 */
static u64 fnv_1a_hash(const void *data, usize dsize, u64 seed)
{
    u8 *data_u8 = (u8 *)data;
    usize nblocks = dsize / 8;

    u64 hash = HASHMAP_HASH_INIT;
    for (size_t i = 0; i < nblocks; ++i)
    {
        hash ^= *(u64 *)data_u8;
        hash *= 0xbf58476d1ce4e5b9;
        data_u8 += 8;
    }

    u64 last = dsize & 0xff;
    switch (dsize % 8)
    {
    case 7:
        last |= (u64)data_u8[6] << 56; /* fallthrough */
    case 6:
        last |= (u64)data_u8[5] << 48; /* fallthrough */
    case 5:
        last |= (u64)data_u8[4] << 40; /* fallthrough */
    case 4:
        last |= (u64)data_u8[3] << 32; /* fallthrough */
    case 3:
        last |= (u64)data_u8[2] << 24; /* fallthrough */
    case 2:
        last |= (u64)data_u8[1] << 16; /* fallthrough */
    case 1:
        last |= (u64)data_u8[0] << 8;
        hash ^= last;
        hash *= 0xd6e8feb86659fd93;
    }

    return hash ^ hash >> 32;
}

/**
 * @brief 释放指针数组中的指针
 *
 * @param ptrs
 * @param len
 */
static inline void free_ptrs(void **ptrs, size len)
{
    for (usize i = 0; i < len; i++)
    {
        free2(ptrs[i]);
    }
}

void hashmap_free(hashmap *map)
{
    if (!map)
    {
        return;
    }
    void *ptrs[7] = {
        map->buckets,
        map->keys,
        map->values,
        map->values_flags,
        map->keys_swap,
        map->values_swap,
        map,
    };
    free_ptrs(ptrs, 7);
}

hashmap *hashmap_new_with_cap(
    usize cap,
    usize ksize,
    usize vsize,
    u64 seed,
    u64 hasher(const void *, usize, u64),
    int cmp(const void *, const void *, usize))
{
    hashmap *map = (hashmap *)malloc(sizeof(hashmap));
    if (map == NULL)
    {
        return NULL;
    }

    map->len = 0;
    map->cap = cap;
    map->resize = (usize)(map->cap * LOAD_FACTOR);
    *(usize *)&map->ksize = ksize;
    *(usize *)&map->vsize = vsize;
    *(usize *)&map->seed = seed;
    map->hasher = (hasher) ? hasher : fnv_1a_hash;
    map->cmp = (cmp) ? cmp : memcmp;
    map->kfree = NULL;
    map->vfree = NULL;

    *(usize *)&map->kdsize = ksize ? ksize : PTR_LEN;
    *(usize *)&map->vdsize = vsize ? vsize : PTR_LEN;
    CMALLOC_CHECK(map->buckets, map->cap, sizeof(bucket), hashmap_free(map));
    MALLOC_CHECK(map->keys, map->kdsize * map->cap, hashmap_free(map));
    MALLOC_CHECK(map->values, map->vdsize * map->cap, hashmap_free(map));
    CMALLOC_CHECK_COND_NULL(map->values_flags, map->cap, sizeof(u8), hashmap_free(map), vsize == 0);
    MALLOC_CHECK(map->keys_swap, map->kdsize * SWAP_CAP, hashmap_free(map));
    MALLOC_CHECK(map->values_swap, map->kdsize * SWAP_CAP, hashmap_free(map));
    return map;
}

hashmap *hashmap_new(
    usize ksize,
    usize vsize,
    u64 seed,
    u64 hasher(const void *, usize, u64),
    int cmp(const void *, const void *, usize))
{
    return hashmap_new_with_cap(INITIAL_BUCKETS, ksize, vsize, seed, hasher, cmp);
}

void hashmap_set_kfree(hashmap *map, void (*kfree)(void *key))
{
    map->kfree = kfree;
}

void hashmap_set_vfree(hashmap *map, void (*vfree)(void *value))
{
    map->vfree = vfree;
}

static inline usize _hashmap_key_size(const hashmap *map)
{
    return map->kdsize;
}

static inline usize _hashmap_val_size(const hashmap *map)
{
    return map->vdsize;
}

static inline void *hashmap_key_p(const hashmap *map, usize index)
{
    return map->keys + (_hashmap_key_size(map) * index);
}

static inline void *hashmap_key_swap_p(const hashmap *map, usize index)
{
    return map->keys_swap + (_hashmap_key_size(map) * index);
}

static inline void *hashmap_value_p(const hashmap *map, usize index)
{
    return map->values + (_hashmap_val_size(map) * index);
}

static inline void *hashmap_value_swap_p(const hashmap *map, usize index)
{
    return map->values_swap + (_hashmap_val_size(map) * index);
}

static inline u8 hashmap_value_flag(const hashmap *map, usize index)
{
    return map->values_flags[index];
}

static inline void hashmap_put_value_flag(const hashmap *map, usize index, u8 flag)
{
    map->values_flags[index] = flag;
}

static inline usize hashmap_hash_index(hashmap *map, u64 hash)
{
    return hash % map->cap;
}

static inline void *hashmap_key(const hashmap *map, usize index)
{
    if (map->ksize == 0)
    {
        return (void *)(*(uintptr_t *)hashmap_key_p(map, index));
    }

    return hashmap_key_p(map, index);
}

static inline void *hashmap_value(const hashmap *map, usize index)
{
    if (map->vsize == 0)
    {
        return (void *)(*(uintptr_t *)hashmap_value_p(map, index));
    }

    return hashmap_value_flag(map, index) ? hashmap_value_p(map, index) : NULL;
}

static void hashmap_free_old_kv(
    hashmap *map,
    bucket *buckets,
    u8 *keys, u8 *vals,
    usize index, usize end_index, usize len, usize end_len)
{
    if (map->kfree || map->vfree)
    {
        while (len < end_len && index < end_index)
        {
            if (buckets[index].psl > 0)
            {
                if (map->kfree)
                {
                    map->kfree(mem_get_val(keys, _hashmap_key_size(map), index));
                }

                if (map->vfree)
                {
                    map->vfree(mem_get_val(vals, _hashmap_val_size(map), index));
                }

                len += 1;
            }

            index += 1;
        }
    }
}

int hashmap_resize(hashmap *map, usize resize)
{
    usize old_len = map->len;
    usize old_cap = map->cap;
    u8 *old_keys = map->keys;
    u8 *old_values = map->values;
    u8 *old_values_flags = map->values_flags;
    bucket *old_buckets = map->buckets;

    map->cap = resize;
    map->keys = (u8 *)malloc(map->kdsize * map->cap);
    map->values = (u8 *)malloc(map->vdsize * map->cap);
    map->values_flags = map->vsize > 0 ? (u8 *)calloc(map->cap, sizeof(u8)) : NULL;
    map->buckets = (bucket *)calloc(map->cap, sizeof(bucket));
    if (!map->keys || !map->values || !map->buckets || (map->vsize > 0 && !map->values_flags))
    {
        free2(map->keys);
        free2(map->values);
        free2(map->values_flags);
        free2(map->buckets);
        map->cap = old_cap;
        map->keys = old_keys;
        map->values = old_values;
        map->values_flags = old_values_flags;
        map->buckets = old_buckets;
        return 1;
    }

    usize i = 0;
    map->len = 0;
    map->resize = (usize)(map->cap * LOAD_FACTOR);
    while (map->len < old_len && i < old_cap)
    {
        if (old_buckets[i].psl > 0)
        {
            hashmap_set(map,
                        mem_get_val(old_keys, _hashmap_key_size(map), i),
                        mem_get_val(old_values, _hashmap_val_size(map), i));
            if (map->len == map->resize)
            {
                break;
            }
        }
        i += 1;
    }

    hashmap_free_old_kv(
        map,
        old_buckets,
        old_keys,
        old_values,
        i,
        old_cap,
        old_len,
        old_cap);
    free2(old_keys);
    free2(old_values);
    free2(old_values_flags);
    free2(old_buckets);
    return 0;
}

static void _hashmap_put_zero_key(hashmap *map, void *key, usize i)
{
    uintptr_t *ptr = hashmap_key_p(map, i);
    *ptr = (uintptr_t)key;
}

static void _hashmap_put_null_key(hashmap *map, void *key, usize i)
{
    u8 *ptr = hashmap_key_p(map, i);
    memset(ptr, 0, _hashmap_key_size(map));
}

static void _hashmap_put_normal_key(hashmap *map, void *key, usize i)
{
    u8 *ptr = hashmap_key_p(map, i);
    memcpy(ptr, key, _hashmap_key_size(map));
}

static void _hashmap_put_zero_value(hashmap *map, void *value, usize i)
{
    uintptr_t *ptr = hashmap_value_p(map, i);
    *ptr = (uintptr_t)value;
}

static void _hashmap_put_null_value(hashmap *map, void *value, usize i)
{
    u8 *ptr = hashmap_value_p(map, i);
    memset(ptr, 0, _hashmap_val_size(map));
    hashmap_put_value_flag(map, i, 0);
}

static void _hashmap_put_normal_value(hashmap *map, void *value, usize i)
{
    u8 *ptr = hashmap_value_p(map, i);
    memcpy(ptr, value, _hashmap_val_size(map));
    hashmap_put_value_flag(map, i, 1);
}

static void *_hashmap_put_zero_key_by_swap(hashmap *map, void *key, usize i, usize swap_i)
{
    uintptr_t *ptr = hashmap_key_p(map, i);
    uintptr_t *swap_k = hashmap_key_swap_p(map, swap_i);
    *swap_k = *ptr;
    *ptr = (uintptr_t)key;
    return (void *)*swap_k;
}

static void *_hashmap_put_null_key_by_swap(hashmap *map, void *key, usize i, usize swap_i)
{
    u8 *ptr = hashmap_key_p(map, i);
    u8 *swap_k = hashmap_key_swap_p(map, swap_i);
    memcpy(swap_k, ptr, _hashmap_key_size(map));
    memset(ptr, 0, _hashmap_key_size(map));
    return swap_k;
}

static void *_hashmap_put_normal_key_by_swap(hashmap *map, void *key, usize i, usize swap_i)
{
    u8 *ptr = hashmap_key_p(map, i);
    u8 *swap_k = hashmap_key_swap_p(map, swap_i);
    memcpy(swap_k, ptr, _hashmap_key_size(map));
    memcpy(ptr, key, _hashmap_key_size(map));
    return swap_k;
}

static void *_hashmap_put_zero_value_by_swap(hashmap *map, void *value, usize i, usize swap_i)
{
    uintptr_t *ptr = hashmap_value_p(map, i);
    uintptr_t *swap_v = hashmap_value_swap_p(map, swap_i);
    *swap_v = *ptr;
    *ptr = (uintptr_t)value;
    return (void *)*swap_v;
}

static void *_hashmap_put_null_value_by_swap(hashmap *map, void *value, usize i, usize swap_i)
{
    u8 *ptr = hashmap_value_p(map, i);
    u8 *swap_v = hashmap_value_swap_p(map, swap_i);
    memcpy(swap_v, ptr, _hashmap_val_size(map));
    memset(ptr, 0, _hashmap_val_size(map));
    hashmap_put_value_flag(map, i, 0);
    return swap_v;
}

static void *_hashmap_put_normal_value_by_swap(hashmap *map, void *value, usize i, usize swap_i)
{
    u8 *ptr = hashmap_value_p(map, i);
    u8 *swap_v = hashmap_value_swap_p(map, swap_i);
    memcpy(swap_v, ptr, _hashmap_val_size(map));
    memcpy(ptr, value, _hashmap_val_size(map));
    hashmap_put_value_flag(map, i, 1);
    return swap_v;
}

static void _hashmap_put_key(hashmap *map, void *key, usize i)
{
    if (map->ksize == 0)
    {
        return _hashmap_put_zero_key(map, key, i);
    }

    if (!key)
    {
        return _hashmap_put_null_key(map, key, i);
    }

    _hashmap_put_normal_key(map, key, i);
}

static void _hashmap_put_value(hashmap *map, void *value, usize i)
{
    if (map->vsize == 0)
    {
        return _hashmap_put_zero_value(map, value, i);
    }

    if (!value)
    {
        return _hashmap_put_null_value(map, value, i);
    }

    _hashmap_put_normal_value(map, value, i);
}

static void *_hashmap_put_key_by_swap(hashmap *map, void *key, usize i, usize swap_i)
{
    if (map->ksize == 0)
    {
        return _hashmap_put_zero_key_by_swap(map, key, i, swap_i);
    }

    if (!key)
    {
        return _hashmap_put_null_key_by_swap(map, key, i, swap_i);
    }

    return _hashmap_put_normal_key_by_swap(map, key, i, swap_i);
}

static void *_hashmap_put_value_by_swap(hashmap *map, void *value, usize i, usize swap_i)
{
    if (map->vsize == 0)
    {
        return _hashmap_put_zero_value_by_swap(map, value, i, swap_i);
    }

    if (!value)
    {
        return _hashmap_put_null_value_by_swap(map, value, i, swap_i);
    }

    return _hashmap_put_normal_value_by_swap(map, value, i, swap_i);
}

typedef struct
{
    usize psl;
    usize i;
    void *key;
    void *value;
    usize swap_i;
    b32 is_exsit;
} _hashmap_insert_t;

static _hashmap_insert_t _hashmap_displacement_instert(hashmap *map, _hashmap_insert_t insert_info)
{
    bucket *b = NULL;
    while (1)
    {
        b = &map->buckets[insert_info.i];

        if (b->psl == 0)
        {
            _hashmap_put_key(map, insert_info.key, insert_info.i);
            _hashmap_put_value(map, insert_info.value, insert_info.i);

            b->psl = insert_info.psl;
            insert_info.psl = 0;
            map->len++;
            return insert_info;
        }

        if (insert_info.psl > b->psl)
        {
            usize swap_i = insert_info.swap_i % SWAP_LEN;
            insert_info.key = _hashmap_put_key_by_swap(map, insert_info.key, insert_info.i, swap_i);
            insert_info.value = _hashmap_put_value_by_swap(map, insert_info.value, insert_info.i, swap_i);

            VALUE_SWAP(b->psl, insert_info.psl);
            insert_info.i = hashmap_hash_index(map, insert_info.i + 1);
            insert_info.swap_i += 1;
            return insert_info;
        }

        insert_info.psl++;
        insert_info.i = hashmap_hash_index(map, insert_info.i + 1);
    }
}

/**
 * @brief hashmap中存在相同key则更新val返回-1, 不存在则返回开始插入的位置
 *
 * @param map
 * @param key
 * @param value
 * @param i hash下标
 * @return 开始查找插入位置的下标
 */
_hashmap_insert_t hashmap_find_insert_index(hashmap *map, void *key, void *value, usize i)
{
    _hashmap_insert_t insert_info = {
        .psl = key ? PSL : NULL_KEY_PSL,
        .i = i,
        .key = key,
        .value = value,
        .swap_i = 0,
        .is_exsit = 0,
    };
    usize psl = PSL;

    bucket b = map->buckets[i];
    if (b.psl == 0)
    {
        return insert_info;
    }

    if (!key || b.psl == NULL_KEY_PSL)
    {
        if (!key && b.psl == NULL_KEY_PSL)
        {
            insert_info.i = i;
            insert_info.is_exsit = 1;
            return insert_info;
        }

        // null key 有最高psl, 要置换存在这里的key
        if (!key)
        {
            return insert_info;
        }

        // b->psl == NULL_KEY_PSL, 即null key, 跳过比较
        psl++;
        i = hashmap_hash_index(map, i + 1);
    }

    while (1)
    {
        b = map->buckets[i];

        if (insert_info.psl == PSL && psl > b.psl)
        {
            insert_info.i = i;
            insert_info.psl = psl;
        }

        if (b.psl == 0)
        {
            return insert_info;
        }

        if (map->cmp(hashmap_key(map, i), key, _hashmap_key_size(map)) == 0)
        {
            insert_info.i = i;
            insert_info.is_exsit = 1;
            return insert_info;
        }

        psl++;
        i = hashmap_hash_index(map, i + 1);
    }
}

/**
 * @brief hashmap插入key val, 函数不判断是否相等, 即默认是新的key, 根据value是否为NULL和vsize是否为0等走特殊处理流程
 *
 * @param map
 * @param key
 * @param value
 * @param i 开始查找插入位置的下标
 * @param psl
 * @return
 */
void hashmap_insert(hashmap *map, void *key, void *value, usize i, usize psl)
{
    _hashmap_insert_t insert_info = {
        .psl = psl,
        .i = i,
        .key = key,
        .value = value,
        .swap_i = 0,
        .is_exsit = 0,
    };

    do
    {
        insert_info = _hashmap_displacement_instert(map, insert_info);
    } while (insert_info.psl > 0);
}

int hashmap_set(hashmap *map, void *key, void *value)
{
    if (!map)
    {
        return 1;
    }

    u64 hash = key ? map->hasher(key, _hashmap_key_size(map), map->seed) : NULL_KEY_HASH;
    usize hash_index = hashmap_hash_index(map, hash);
    _hashmap_insert_t insert_info = hashmap_find_insert_index(map, key, value, hash_index);
    if (insert_info.is_exsit)
    {
        _hashmap_put_value(map, value, insert_info.i);
        return 0;
    }

    if (map->len == map->resize)
    {
        if (hashmap_resize(map, map->cap * RESIZE_ZOOM))
        {
            return 1;
        }
        insert_info.i = hashmap_hash_index(map, hash);
        insert_info.psl = key ? PSL : NULL_KEY_PSL;
    }
    hashmap_insert(map, key, value, insert_info.i, insert_info.psl);
    return 0;
}

void *hashmap_get(hashmap *map, const void *key)
{
    if (!map || map->len == 0)
    {
        return NULL;
    }

    u64 hash = key ? map->hasher(key, _hashmap_key_size(map), map->seed) : NULL_KEY_HASH;
    usize hash_index = hashmap_hash_index(map, hash);
    _hashmap_insert_t insert_info = hashmap_find_insert_index(map, (void *)key, NULL, hash_index);
    if (insert_info.is_exsit)
    {
        return hashmap_value(map, insert_info.i);
    }

    return NULL;
}

void *hashmap_get_clone(hashmap *map, const void *key)
{
    void *val = hashmap_get(map, key);
    if (!val)
    {
        return NULL;
    }

    void *ret_val;
    MALLOC_CHECK(ret_val, _hashmap_val_size(map), )
    memcpy(ret_val, val, _hashmap_val_size(map));
    return ret_val;
}

b32 hashmap_exist(hashmap *map, const void *key)
{
    if (!map || map->len == 0)
    {
        return 0;
    }

    u64 hash = key ? map->hasher(key, _hashmap_key_size(map), map->seed) : NULL_KEY_HASH;
    usize hash_index = hashmap_hash_index(map, hash);
    _hashmap_insert_t insert_info = hashmap_find_insert_index(map, (void *)key, NULL, hash_index);
    if (insert_info.is_exsit)
    {
        return 1;
    }

    return 0;
}

static void hashmap_free_kv(hashmap *map, usize i)
{
    if (map->kfree)
    {
        map->kfree(hashmap_key(map, i));
    }

    if (map->vfree)
    {
        map->vfree(hashmap_value(map, i));
    }
}

static void hashmap_remove_i(hashmap *map, usize i)
{
    bucket *b, *pre_b;

    hashmap_free_kv(map, i);
    pre_b = &map->buckets[i];
    void *pre_k = hashmap_key_p(map, i);
    void *pre_v = hashmap_value_p(map, i);
    void *cur_k = NULL;
    void *cur_v = NULL;
    while (1)
    {
        i = (i + 1) % map->cap;
        b = &map->buckets[i];
        if (b->psl <= 1)
        {
            // 标记删除
            pre_b->psl = 0;
            break;
        }
        cur_k = hashmap_key_p(map, i);
        cur_v = hashmap_value_p(map, i);
        memcpy(pre_k, cur_k, _hashmap_key_size(map));
        memcpy(pre_v, cur_v, _hashmap_val_size(map));
        pre_k = cur_k;
        pre_v = cur_v;

        pre_b->psl = b->psl - 1;
        pre_b = b;
    };

    map->len--;
}

int hashmap_remove(hashmap *map, const void *key)
{
    if (!map)
    {
        return 1;
    }

    if (map->len == 0)
    {
        return 0;
    }

    u64 hash = key ? map->hasher(key, _hashmap_key_size(map), map->seed) : NULL_KEY_HASH;
    usize hash_index = hashmap_hash_index(map, hash);

    _hashmap_insert_t insert_info = hashmap_find_insert_index(map, (void *)key, NULL, hash_index);
    if (!insert_info.is_exsit)
    {
        return 0;
    }

    hashmap_remove_i(map, insert_info.i);
    return 0;
}

size hashmap_count(hashmap *map)
{
    if (!map)
    {
        return 0;
    }

    return map->len;
}

int hashmap_clear(hashmap *map)
{
    if (!map)
    {
        return 1;
    }

    usize i = 0;
    while (map->len > 0 && i < map->cap)
    {
        bucket *b = &map->buckets[i++];
        if (b->psl > 0)
        {
            b->psl = 0;
            map->len--;
            hashmap_free_kv(map, i);
        }
    }
    assert(map->len == 0 && "hashmap_clear error");
    return 0;
}

int hashmap_update(hashmap *dst, hashmap *src)
{
    if (!dst || !src)
    {
        return 1;
    }

    usize i = 0;
    usize l = src->len;
    while (i < src->cap && l > 0)
    {
        if (src->buckets[i].psl > 0)
        {
            hashmap_set(dst, hashmap_key(src, i), hashmap_value(src, i));
            l--;
        }
        i++;
    }

    return 0;
}

hashmap *hashmap_clone(hashmap *map)
{
    if (!map)
    {
        return NULL;
    }

    hashmap *new_map = hashmap_new_with_cap(
        map->cap,
        map->ksize,
        map->vsize,
        map->seed,
        map->hasher,
        map->cmp);

    hashmap_update(new_map, map);
    return new_map;
}

inline b32 hashmap_empty(hashmap *map)
{
    return map->len == 0;
}

hashmap_iterator hashmap_begin(hashmap *map)
{
    hashmap_iterator iter = {
        .map = map,
        .index = 0,
        .step = 1,
        .len = 0,
        .state = 0,
    };

    return iter;
}

hashmap_iterator hashmap_end(hashmap *map)
{
    hashmap_iterator iter = {
        .map = map,
        .index = map->cap - 1,
        .step = -1,
        .len = 0,
        .state = 0,
    };

    return iter;
}

b32 hashmap_iter_is_end(hashmap_iterator *iter)
{
    if (!iter)
    {
        return 1;
    }

    if (iter->len == iter->map->len)
    {
        return 1;
    }

    if (iter->step < 0)
    {
        return iter->index < 0;
    }

    return iter->index >= iter->map->cap;
}

void *hashmap_iter_key(hashmap_iterator *iter)
{
    if (!iter || !iter->map || hashmap_iter_is_end(iter))
    {
        return NULL;
    }

    const hashmap *map = iter->map;
    do
    {
        if (map->buckets[iter->index].psl > 0)
        {
            void *key = hashmap_key(map, iter->index);
            iter->index += iter->step;
            iter->len += 1;
            return key;
        }
        iter->index += iter->step;
    } while (!hashmap_iter_is_end(iter));

    return NULL;
}

void *hashmap_iter_value(hashmap_iterator *iter)
{
    if (!iter || !iter->map || hashmap_iter_is_end(iter))
    {
        return NULL;
    }

    const hashmap *map = iter->map;
    do
    {
        if (map->buckets[iter->index].psl > 0)
        {
            void *val = hashmap_value(map, iter->index);
            iter->index += iter->step;
            iter->len += 1;
            return val;
        }
        iter->index += iter->step;
    } while (!hashmap_iter_is_end(iter));

    return NULL;
}

hashmap_iterator_kv hashmap_iter_kv(hashmap_iterator *iter)
{
    hashmap_iterator_kv kv = {1, NULL, NULL};
    if (!iter || !iter->map || hashmap_iter_is_end(iter))
    {
        return kv;
    }

    const hashmap *map = iter->map;
    do
    {
        if (map->buckets[iter->index].psl > 0)
        {
            kv.state = 0;
            kv.key = hashmap_key(map, iter->index);
            kv.value = hashmap_key(map, iter->index);
            iter->index += iter->step;
            iter->len += 1;
            return kv;
        }
        iter->index += iter->step;
    } while (!hashmap_iter_is_end(iter));

    return kv;
}