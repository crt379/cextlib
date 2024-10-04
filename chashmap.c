#include "chashmap.h"
#include "ctype.h"

#define SWAP_CAP 2 // swap key value 的容量，不只是用于交换
#define SWAP_LEN 2 // 交换使用的长度

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

inline void free_ptrs(void **ptrs, size len)
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
    void *ptrs[6] = {
        map->keys,
        map->values,
        map->buckets,
        map->keys_swap,
        map->values_swap,
        map,
    };
    free_ptrs(ptrs, 6);
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

    CMALLOC_CHECK(map->buckets, map->cap, sizeof(bucket), hashmap_free(map));
    MALLOC_CHECK(map->keys, ksize * map->cap, hashmap_free(map));
    MALLOC_CHECK_COND_NULL(map->values, vsize * map->cap, hashmap_free(map), vsize == 0);

    MALLOC_CHECK(map->keys_swap, ksize * SWAP_CAP, hashmap_free(map));
    MALLOC_CHECK_COND_NULL(map->values_swap, vsize * SWAP_CAP, hashmap_free(map), vsize == 0);

    *(usize *)&map->ksize = ksize;
    *(usize *)&map->vsize = vsize;
    *(usize *)&map->seed = seed;
    map->hasher = (hasher) ? hasher : fnv_1a_hash;
    map->cmp = (cmp) ? cmp : memcmp;
    return map;
}

hashmap *hashmap_new(
    usize ksize,
    usize vsize,
    u64 seed,
    u64 hasher(const void *, usize, u64),
    int cmp(const void *, const void *, usize))
{
    assert(ksize > 0 && "ksize must be greater than 0");
    return hashmap_new_with_cap(INITIAL_BUCKETS, ksize, vsize, seed, hasher, cmp);
}

int hashmap_resize(hashmap *map, usize resize)
{
    usize old_len = map->len;
    usize old_cap = map->cap;
    u8 *old_keys = map->keys;
    u8 *old_values = map->values;
    bucket *old_buckets = map->buckets;

    map->cap = resize;
    map->keys = (u8 *)malloc(map->ksize * map->cap);
    map->values = map->vsize > 0 ? (u8 *)malloc(map->vsize * map->cap) : NULL;
    map->buckets = (bucket *)calloc(map->cap, sizeof(bucket));

    if (!map->keys || !map->values || !map->buckets)
    {
        free2(map->keys);
        free2(map->values);
        free2(map->buckets);
        map->cap = old_cap;
        map->keys = old_keys;
        map->values = old_values;
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
            hashmap_set(map, old_buckets[i].key, old_buckets[i].value);
            if (map->len == map->resize)
            {
                break;
            }
        }
        i += 1;
    }

    free2(old_keys);
    free2(old_values);
    free2(old_buckets);
    return 0;
}

usize hashmap_hash_index(hashmap *map, u64 hash)
{
    return hash % map->cap;
}

static void _hashmap_put_key(hashmap *map, bucket *b, void *key, usize i)
{
    u8 *ptr = map->keys + (i * map->ksize);
    memcpy(ptr, key, map->ksize);
    b->key = ptr;
}

static void _hashmap_put_zero_value(hashmap *map, bucket *b, void *value, usize i)
{
    b->value = NULL;
}

static void _hashmap_put_null_value(hashmap *map, bucket *b, void *value, usize i)
{
    u8 *ptr = map->values + (i * map->vsize);
    memset(ptr, 0, map->vsize);
    b->value = NULL;
}

static void _hashmap_put_normal_value(hashmap *map, bucket *b, void *value, usize i)
{
    u8 *ptr = map->values + (i * map->vsize);
    memcpy(ptr, value, map->vsize);
    b->value = ptr;
}

static void _hashmap_put_value(hashmap *map, bucket *b, void *value, usize i)
{
    if (map->vsize == 0)
    {
        return _hashmap_put_zero_value(map, b, value, i);
    }

    if (!value)
    {
        return _hashmap_put_null_value(map, b, value, i);
    }

    _hashmap_put_normal_value(map, b, value, i);
}

size hashmap_update_or_insert_index(hashmap *map, void *key, void *value, usize i)
{
    u64 psl = PSL;
    usize ii = i; // 插入下标
    bucket *b = NULL;
    while (1)
    {
        b = &map->buckets[i];

        if (psl == PSL && psl > b->psl)
        {
            ii = i;
        }

        if (b->psl == 0)
        {
            return ii;
        }

        if (map->cmp(b->key, key, map->ksize) == 0)
        {
            _hashmap_put_key(map, b, key, i);
            _hashmap_put_value(map, b, value, i);
            return -1;
        }

        psl++;
        i = hashmap_hash_index(map, i + 1);
    }
}

static void *_hashmap_put_key_by_swap(hashmap *map, bucket *b, void *key, usize i, usize swap_i)
{
    u8 *ptr = ptr = map->keys + (i * map->ksize);
    u8 *swap_v = map->keys_swap + (map->ksize * swap_i);
    memcpy(swap_v, ptr, map->ksize);
    memcpy(ptr, key, map->ksize);
    b->key = ptr;
    return swap_v;
}

static void *_hashmap_put_zero_value_by_swap(hashmap *map, bucket *b, void *value, usize i, usize swap_i)
{
    _hashmap_put_zero_value(map, b, NULL, i);
    return NULL;
}

static void *_hashmap_put_null_value_by_swap(hashmap *map, bucket *b, void *value, usize i, usize swap_i)
{
    u8 *ptr = ptr = map->values + (i * map->vsize);
    u8 *swap_v = map->values_swap + (map->vsize * swap_i);
    memcpy(swap_v, ptr, map->vsize);
    memset(ptr, 0, map->vsize);
    b->value = NULL;
    return swap_v;
}

static void *_hashmap_put_normal_value_by_swap(hashmap *map, bucket *b, void *value, usize i, usize swap_i)
{
    u8 *ptr = map->values + (i * map->vsize);
    u8 *swap_v = map->values_swap + (map->vsize * swap_i);
    memcpy(swap_v, ptr, map->vsize);
    memcpy(ptr, value, map->vsize);
    b->value = ptr;
    return swap_v;
}

static void *_hashmap_put_value_by_swap(hashmap *map, bucket *b, void *value, usize i, usize swap_i)
{
    if (map->vsize == 0)
    {
        return _hashmap_put_zero_value_by_swap(map, b, value, i, swap_i);
    }

    if (!value)
    {
        return _hashmap_put_null_value_by_swap(map, b, value, i, swap_i);
    }

    return _hashmap_put_normal_value_by_swap(map, b, value, i, swap_i);
}

typedef struct
{
    u64 psl;
    usize i;
    void *key;
    void *value;
} _hashmap_disp_insert_t;

static _hashmap_disp_insert_t _hashmap_displacement_instert(hashmap *map, void *key, void *value, usize i, u64 psl)
{
    bucket *b = NULL;
    usize swap_len = SWAP_LEN;
    usize swap_cur = 0;
    _hashmap_disp_insert_t ret = {0};

    while (1)
    {
        b = &map->buckets[i];

        if (b->psl == 0)
        {
            b->psl = psl;
            _hashmap_put_key(map, b, key, i);
            _hashmap_put_value(map, b, value, i);
            return ret;
        }

        if (psl > b->psl)
        {
            usize swap_i = swap_cur++ % swap_len;
            ret.key = _hashmap_put_key_by_swap(map, b, key, i, swap_i);
            ret.value = _hashmap_put_value_by_swap(map, b, value, i, swap_i);

            ret.psl = b->psl;
            b->psl = psl;
            ret.i = hashmap_hash_index(map, i + 1);
            return ret;
        }

        psl++;
        i = hashmap_hash_index(map, i + 1);
    }
}

// hashmap_insert(hashmap *map, void *key, void *value, usize i, u64 psl)
#define __HASHMAP_INSERT(BREAK_VAL_P_H, SWAP_VAL_P_H)                   \
    {                                                                   \
        bucket *b = NULL;                                               \
        usize swap_len = SWAP_LEN;                                      \
        usize swap_cur = 0;                                             \
        while (1)                                                       \
        {                                                               \
            b = &map->buckets[i];                                       \
                                                                        \
            if (b->psl == 0)                                            \
            {                                                           \
                map->len++;                                             \
                b->psl = psl;                                           \
                _hashmap_put_key(map, b, key, i);                       \
                                                                        \
                BREAK_VAL_P_H                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            if (psl > b->psl)                                           \
            {                                                           \
                usize swap_i = swap_cur++ % swap_len;                   \
                key = _hashmap_put_key_by_swap(map, b, key, i, swap_i); \
                                                                        \
                SWAP_VAL_P_H                                            \
                                                                        \
                u64 tmp_psl = b->psl;                                   \
                b->psl = psl;                                           \
                psl = tmp_psl;                                          \
            }                                                           \
                                                                        \
            psl++;                                                      \
            i = hashmap_hash_index(map, i + 1);                         \
        }                                                               \
    }

/**
 * @brief hashmap插入key, value为0长度情况, 函数不判断是否相等, 即默认是新的key
 *
 * @param map
 * @param key
 * @param value
 * @param hashi hash下标
 * @param i 开始查找插入位置的下标
 * @return
 */
static void hashmap_insert_zero_value(hashmap *map, void *key, void *value, usize i, u64 psl)
{
#define __BREAK_VAL_P_H _hashmap_put_zero_value(map, b, NULL, i);

#define ___SWAP_VAL_P_H _hashmap_put_zero_value(map, b, NULL, i);

    __HASHMAP_INSERT(__BREAK_VAL_P_H, ___SWAP_VAL_P_H);
#undef __BREAK_VAL_P_H
#undef ___SWAP_VAL_P_H
}

/**
 * @brief hashmap插入key, value为NULL情况, 函数不判断是否相等, 即默认是新的key
 *
 * @param map
 * @param key
 * @param hashi hash下标
 * @param i 开始查找插入位置的下标
 * @return
 */
static void hashmap_insert_null_value(hashmap *map, void *key, void *value, usize i, u64 psl)
{
#define __BREAK_VAL_P_H _hashmap_put_null_value(map, b, value, i);

#define ___SWAP_VAL_P_H value = _hashmap_put_null_value_by_swap(map, b, value, i, swap_i);

    __HASHMAP_INSERT(__BREAK_VAL_P_H, ___SWAP_VAL_P_H);
#undef __BREAK_VAL_P_H
#undef ___SWAP_VAL_P_H
}

/**
 * @brief hashmap插入key val, 函数不判断是否相等, 即默认是新的key
 *
 * @param map
 * @param key
 * @param value
 * @param hashi hash下标
 * @param i 开始查找插入位置的下标
 * @return
 */
static void hashmap_insert_normal_value(hashmap *map, void *key, void *value, usize i, u64 psl)
{
#define __BREAK_VAL_P_H _hashmap_put_normal_value(map, b, value, i);

#define ___SWAP_VAL_P_H value = _hashmap_put_normal_value_by_swap(map, b, value, i, swap_i);

    __HASHMAP_INSERT(__BREAK_VAL_P_H, ___SWAP_VAL_P_H);
#undef __BREAK_VAL_P_H
#undef ___SWAP_VAL_P_H
}

void hashmap_insert(hashmap *map, void *key, void *value, usize i, u64 psl)
{
    if (map->vsize == 0)
    {
        return hashmap_insert_zero_value(map, key, value, i, psl);
    }

    if (value == NULL)
    {
        return hashmap_insert_null_value(map, key, value, i, psl);
    }

    return hashmap_insert_normal_value(map, key, value, i, psl);
}

static u64 _hashmap_psl_by_hashi_i(hashmap *map, usize hashi, usize i)
{
    return (hashi > i) ? map->cap + i - hashi + PSL : i - hashi + PSL;
}

int hashmap_set(hashmap *map, void *key, void *value)
{
    if (!map)
    {
        return 1;
    }

    u64 hash = map->hasher(key, map->ksize, map->seed);
    usize hash_index = hashmap_hash_index(map, hash);

    // 查找更新 或 查找可以插入的位置
    size inse_index = hashmap_update_or_insert_index(map, key, value, hash_index);
    if (inse_index < 0)
    {
        return 0;
    }

    if (map->len == map->resize)
    {
        if (hashmap_resize(map, map->cap * RESIZE_ZOOM))
        {
            return 1;
        }
        hash_index = hashmap_hash_index(map, hash);
        hashmap_insert(map, key, value, hash_index, _hashmap_psl_by_hashi_i(map, hash_index, hash_index));
    }
    else
    {
        hashmap_insert(map, key, value, inse_index, _hashmap_psl_by_hashi_i(map, hash_index, inse_index));
    }

    return 0;
}

void *hashmap_get(hashmap *map, const void *key)
{
    if (!map || map->len == 0)
    {
        return NULL;
    }

    bucket *b = NULL;
    usize i = map->hasher(key, map->ksize, map->seed) % map->cap;
    while (1)
    {
        b = &map->buckets[i];
        if (b->psl == 0)
        {
            return NULL;
        }

        if (map->cmp(b->key, key, map->ksize) == 0)
        {
            return b->value;
        }

        i = (i + 1) % map->cap;
    }

    return NULL;
}

void *hashmap_get_cp(hashmap *map, const void *key)
{
    void *val = hashmap_get(map, key);
    if (!val)
    {
        return NULL;
    }

    void *ret_val = malloc(map->vsize);
    if (!ret_val)
    {
        return NULL;
    }
    memcpy(ret_val, val, map->vsize);
    return ret_val;
}

b32 hashmap_exist(hashmap *map, const void *key)
{
    if (!map || map->len == 0)
    {
        return 0;
    }

    bucket *b = NULL;
    usize i = map->hasher(key, map->ksize, map->seed) % map->cap;
    while (1)
    {
        b = &map->buckets[i];
        if (b->psl == 0)
        {
            return 0;
        }

        if (map->cmp(b->key, key, map->ksize) == 0)
        {
            return 1;
        }

        i = (i + 1) % map->cap;
    }

    return 0;
}

int hashmap_remove(hashmap *map, const void *key)
{
    if (!map || !key)
    {
        return 1;
    }

    if (map->len == 0)
    {
        return 0;
    }

    bucket *b = NULL;
    usize i = map->hasher(key, map->ksize, map->seed) % map->cap;
    while (1)
    {
        b = &map->buckets[i];
        if (b->psl == 0)
        {
            return 0;
        }

        if (map->cmp(b->key, key, map->ksize) == 0)
        {
            map->len--;
            break;
        }

        i = (i + 1) % map->cap;
    }

    // usize pre_i = i;
    bucket *pre_b = b;
    void *pre_k = map->keys + (i * map->ksize);
    void *pre_v = map->values + (i * map->vsize);

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
        cur_k = map->keys + (i * map->ksize);
        cur_v = map->values + (i * map->vsize);

        memcpy(pre_k, cur_k, map->ksize);
        memcpy(pre_v, cur_v, map->vsize);
        pre_b->psl = b->psl - 1;
        pre_b->key = pre_k;   // 可能可以删掉这行
        pre_b->value = pre_v; // 可能可以删掉这行

        // pre_i = i;
        pre_b = b;
        pre_k = cur_k;
        pre_v = cur_v;
    };

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
        bucket *b = &src->buckets[i++];
        if (b->psl > 0)
        {
            l--;
            hashmap_set(dst, b->key, b->value);
        }
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
    while (!hashmap_iter_is_end(iter))
    {
        bucket *b = &map->buckets[iter->index];
        iter->index += iter->step;
        if (b->psl > 0)
        {
            iter->len++;
            return b->key;
        }
    }

    return NULL;
}

void *hashmap_iter_value(hashmap_iterator *iter)
{
    if (!iter || !iter->map || hashmap_iter_is_end(iter))
    {
        return NULL;
    }

    const hashmap *map = iter->map;
    while (!hashmap_iter_is_end(iter))
    {
        bucket *b = &map->buckets[iter->index];
        iter->index += iter->step;
        if (b->psl > 0)
        {
            iter->len++;
            return b->value;
        }
    }

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
    while (!hashmap_iter_is_end(iter))
    {
        bucket *b = &map->buckets[iter->index];
        iter->index += iter->step;
        if (b->psl > 0)
        {
            iter->len++;
            kv.state = 0;
            kv.key = b->key;
            kv.value = b->value;
            return kv;
        }
    }

    return kv;
}