
#ifndef __CVEC_H
#define __CVEC_H

#include <string.h>
#include "ctype.h"

#define vec_new_bind(bind_p) vec_bind_new(sizeof(*bind_p), (void*)&bind_p)
#define vec_push_v(h, v) ({ \
    __typeof__(v) temp = (v); \
    vec_push(h, &temp); \
})
#define vec_put_v(h, v, i) ({ \
    __typeof__(v) temp = (v); \
    vec_put(h, &temp, i); \
})
#define vec_insert_v(h, v, i) ({ \
    __typeof__(v) temp = (v); \
    vec_insert(h, &temp, i); \
})
#define vec_find_v(h, v, f) ({ \
    __typeof__(v) temp = (v); \
    vec_find(h, &temp, f); \
})
#define vec_find_default_v(h, v) ({ \
    __typeof__(v) temp = (v); \
    vec_find_default(h, &temp); \
})

typedef struct vec_header {
    u32 len;
    u32 cap;
    const u32 vsize;
    const u32 min_non_zero_cap;
    void* bind_pp;
    void* mem;
} vec;

/**
 * 计算一个非零容量的最小值
 * 
 * @param vsize 
 */
u32 vec_min_non_zero_cap(u32 vsize);

/**
 * @brief 创建一个空的 vec
 * 
 * @param vsize 元素大小
 * @param len
 * @param cap
 * @param bind_pp 绑定数据区的指针的指针
 * @param memory 内存指针
 * @return vec*
 */
vec* vec_create(u32 vsize, u32 len, u32 cap, void* bind_pp, void* memory);

/**
 * @brief 创建一个空的 vec
 * 
 * @param vsize 元素大小
 * @return vec*
 */
inline vec* vec_new(u32 vsize)
{
    return vec_create(vsize, 0, 0, NULL, NULL);
}

/**
 * @brief 创建一个空的 vec
 * 
 * @param vsize 元素大小
 * @param bind_pp 绑定数据区的指针的指针
 * @return vec*
 */
inline vec* vec_bind_new(u32 vsize, void* bind_pp)
{
    return vec_create(vsize, 0, 0, bind_pp, NULL);
}

/**
 * @brief 设置绑定 vec 内存到指针
 * 
 * @param h 
 * @param bind_pp 
 */
void vec_set_bind_pp(vec* h, void* bind_pp);

/**
 * @brief 扩容
 * 
 * @param h 
 * @param cap 
 * @return int 成功 0，失败 1
 */
int vec_cap_expansion(vec* h, size cap);

/**
 * @brief 缩容
 * 
 * @param h 
 * @param cap 
 * @return int 成功 0，失败 1
 */
int vec_cap_scaling(vec* h, size cap);

/**
 * @brief 重新设置cap
 * 
 * @param h 
 * @param cap 
 * @return int 成功 0，失败 1
 */
int vec_resize(vec* h, size cap);

/**
 * @brief 插入元素
 * 
 * @param h 
 * @param v 
 * @return usize 返回插入位置, 失败返回-1
 */
size vec_push(vec* h, void* v);

/**
 * @brief 修改对应位置元素
 * 
 * @param h 
 * @param v 
 * @param i 
 * @return int 成功0 失败1
 */
int vec_put(vec* h, void* v, size i);

/**
 * @brief 插入元素到指定位置
 * 
 * @param h 
 * @param v 
 * @param i 
 * @return int 成功0 失败1
 */
int vec_insert(vec* h, void* v, size i);

/**
 * @brief 从数组创建 vec
 * 
 * @param vsize 
 * @param src 
 * @param len 
 * @return vec* 
 */
vec* vec_from(u32 vsize, void* src, u32 len);

/**
 * @brief 获取对应位置元素
 * 
 * @param h 
 * @param i 
 * @return void* 
 */
void* vec_get(vec* h, size i);

/**
 * @brief 删除最后一个元素
 * 
 * @param h 
 */
void vec_pop(vec* h);

/**
 * @brief 删除对应位置元素
 * 
 * @param h 
 * @param i 
 * @return int 成功0 失败1
 */
int vec_remove(vec* h, size i);

/**
 * @brief 清空数组
 * 
 * @param h 
 * @return int 
 */
inline void vec_clear(vec* h)
{
    if (!h) return;
    h->len = 0;
}

/**
 * @brief 释放内存
 * 
 * @param h 
 */
void vec_free(vec* h);

/**
 * @brief 创建切片
 * 
 * @param h 
 * @param start 
 * @param end 
 * @return vec* 
 */
vec* vec_slice(vec* h, usize start, usize end);

/**
 * @brief 复制数组
 * 
 * @param h 
 * @return vec* 
 */
vec* vec_clone(vec* h);

/**
 * @brief 查找元素
 * 
 * @param h 
 * @param v 
 * @param func 
 * @return size 
 */
size vec_find(vec* h, void* v, b32 (*func)(void*, void*));

/**
 * @brief 查找元素，从尾部开始
 * 
 * @param h 
 * @param v 
 * @param func 
 * @return size 
 */
size vec_find_last(vec* h, void* v, b32 (*func)(void*, void*));

/**
 * @brief 默认比较函数
 * 
 * @param a 
 * @param b 
 * @param vsize 
 * @return b32 
 */
b32 default_comparison(void* a, void* b, size vsize);

/**
 * @brief 默认查找函数
 * 
 * @param h 
 * @param v 
 * @return size 
 */
size vec_find_default(vec* h, void* v);

/**
 * @brief 默认查找函数，从尾部开始
 * 
 * @param h 
 * @param v 
 * @return size 
 */
size vec_find_default_last(vec* h, void* v);

#endif // __CVEC_H