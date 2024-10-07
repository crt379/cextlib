#ifndef __CHASHMAP_H
#define __CHASHMAP_H

#include "ctype.h"

#define INITIAL_BUCKETS 16
#define LOAD_FACTOR     0.8
#define RESIZE_ZOOM     1.5

/**
 * @brief 检查hashmap操作是否成功
 *
 * @param ret
 * @return b32
 */
inline b32 hashmap_is_success(int ret)
{
    return ret == 0;
}

// ============================================================================
// hashmap
// ============================================================================

typedef struct bucket
{
    i64 psl;
    // void *key;
    // void *value;
} bucket;

typedef struct hashmap_header
{
    // 数据
    bucket *buckets;
    u8 *keys;
    u8 *values;
    u8 *values_flags;
    // 容量
    usize cap;
    usize len;
    usize resize;
    const usize ksize;
    const usize vsize;
    const usize kdsize;
    const usize vdsize;
    const u64 seed;
    // 交换使用
    u8 *keys_swap;
    u8 *values_swap;
    // 动态函数
    u64 (*hasher)(const void *data, usize dsize, u64 seed);
    int (*cmp)(const void *key1, const void *key2, usize ksize);
} hashmap;

/**
 * @brief hashmap释放
 *
 * @param map
 */
void hashmap_free(hashmap *map);

/**
 * @brief 创建hashmap
 *
 * @param cap 初始容量
 * @param ksize key大小
 * @param vsize value大小
 * @param seed 随机种子
 * @param hasher hash函数
 * @param cmp 比较函数
 * @return 返回新创建的hashmap指针，如果内存分配失败或参数检查失败则返回NULL
 */
hashmap *hashmap_new_with_cap(
    usize cap,
    usize ksize,
    usize vsize,
    u64 seed,
    u64 hasher(const void *, usize, u64),
    int cmp(const void *, const void *, usize));

/**
 * @brief 创建hashmap
 *
 * @param ksize key大小
 * @param vsize value大小
 * @param seed 随机种子
 * @param hasher hash函数
 * @param cmp 比较函数
 * @return 返回新创建的hashmap指针，如果内存分配失败或参数检查失败则返回NULL
 */
hashmap *hashmap_new(
    usize ksize,
    usize vsize,
    u64 seed,
    u64 hasher(const void *, usize, u64),
    int cmp(const void *, const void *, usize));

/**
 * @brief hashmap插入key val
 *
 * @param map
 * @param key
 * @param value
 * @return 成功返回0 失败返回非0
 */
int hashmap_set(hashmap *map, void *key, void *value);

/**
 * @brief hashmap重新设置大小, resize小于原始容量可能会丢弃某些数据
 *
 * @param map
 * @param resize
 * @return 成功返回0 失败返回非0
 */
int hashmap_resize(hashmap *map, usize resize);

/**
 * @brief hashmap查找key
 *
 * @param map
 * @param key
 * @return 查找到返回val所在指针 否则返回NULL
 */
void *hashmap_get(hashmap *map, const void *key);

/**
 * @brief hashmap查找key, 并拷贝对应值
 *
 * @param map
 * @param key
 * @return 查找到返回拷贝val所在指针 否则返回NULL
 */
void *hashmap_get_cp(hashmap *map, const void *key);

/**
 * @brief hashmap查找key是否存在
 *
 * @param map
 * @param key
 * @return 存在返回1 否则返回0
 */
b32 hashmap_exist(hashmap *map, const void *key);

/**
 * @brief hashmap移除元素
 *
 * @param map
 * @param key
 * @return 成功返回0 失败返回非0
 */
int hashmap_remove(hashmap *map, const void *key);

/**
 * @brief hashmap元素个数
 *
 * @param map
 * @return 返回hashmap元素个数
 */
size hashmap_count(hashmap *map);

/**
 * @brief hashmap清空
 *
 * @param map
 * @return 成功返回0 失败返回非0
 */
int hashmap_clear(hashmap *map);

/**
 * @brief hashmap合并
 *
 * @param dst
 * @param src
 * @return 成功返回0 失败返回非0
 */
int hashmap_update(hashmap *dst, hashmap *src);

/**
 * @brief hashmap克隆
 *
 * @param map
 * @return 成功返回hashmap指针 失败返回NULL
 */
hashmap *hashmap_clone(hashmap *map);

/**
 * @brief hashmap是否为空
 *
 * @param map
 * @return 1为空 0不为空
 */
b32 hashmap_empty(hashmap *map);

// ============================================================================
//  hashmap迭代器
// ============================================================================

typedef struct
{
    const hashmap *map;
    usize index;
    const size step;
    usize len;
    int state;
} hashmap_iterator;

/**
 * @brief hashmap迭代器初始化
 *
 * @param map
 * @return hashmap_iterator
 */
hashmap_iterator hashmap_begin(hashmap *map);

/**
 * @brief hashmap迭代器从结尾开始
 *
 * @param map
 * @return hashmap_iterator
 */
hashmap_iterator hashmap_end(hashmap *map);

/**
 * @brief hashmap迭代器是否结束
 *
 * @param iter
 * @return 1为结束 0为未结束
 */
b32 hashmap_iter_is_end(hashmap_iterator *iter);

/**
 * @brief hashmap迭代器获取key
 *
 * @param iter
 * @return void*
 */
void *hashmap_iter_key(hashmap_iterator *iter);

/**
 * @brief hashmap迭代器获取value
 *
 * @param iter
 * @return void*
 */
void *hashmap_iter_value(hashmap_iterator *iter);

typedef struct
{
    int state;
    void *key;
    void *value;
} hashmap_iterator_kv;

/**
 * @brief hashmap迭代器获取key和value
 *
 * @param iter
 * @return hashmap_iterator_kv
 */
hashmap_iterator_kv hashmap_iter_kv(hashmap_iterator *iter);

#endif // __CHASHMAP_H