#include "cvec.h"
u32 vec_min_non_zero_cap(u32 vsize)
{
    if (vsize == 1)
    {
        return 8;
    }
    else if (vsize <= 1024)
    {
        return 4;
    }
    else
    {
        return 1;
    }
}

vec *vec_create(u32 vsize, u32 len, u32 cap, void *bind_pp, void *memory)
{
    vec *h = (vec *)malloc(sizeof(vec));
    h->len = len;
    h->cap = cap;
    // stackoverflow.com/questions/9691404/how-to-initialize-const-in-a-struct-in-c-with-malloc
    *(u32 *)&h->vsize = vsize;
    *(u32 *)&h->min_non_zero_cap = vec_min_non_zero_cap(vsize);
    h->bind_pp = bind_pp;
    h->mem = memory;
    return h;
}

vec *vec_new(u32 vsize)
{
    return vec_create(vsize, 0, 0, NULL, NULL);
}

vec *vec_bind_new(u32 vsize, void *bind_pp)
{
    return vec_create(vsize, 0, 0, bind_pp, NULL);
}

/**
 * @brief 绑定 vec 内存到指针
 *
 * @param h
 */
void vec_bind_pp_bind_mem(vec *h);

inline void vec_bind_pp_bind_mem(vec *h)
{
    if (h->bind_pp)
    {
        *(u8 **)h->bind_pp = (u8 *)h->mem;
    };
}

void vec_set_bind_pp(vec *h, void *bind_pp)
{
    h->bind_pp = bind_pp;
    vec_bind_pp_bind_mem(h);
}

/**
 * @brief 替换为新内存并替换，然后返回旧内存
 *
 * @param h
 * @param cap
 * @return void*
 */
void *vec_cap_replace_mem(vec *h, size cap);

inline void *vec_cap_replace_mem(vec *h, size cap)
{
    void *old_mem = h->mem;

    h->cap = cap;
    h->mem = malloc(h->vsize * h->cap);
    vec_bind_pp_bind_mem(h);
    return old_mem;
}

int vec_cap_expansion(vec *h, size cap)
{
    void *old_mem = vec_cap_replace_mem(h, cap);
    if (!h->mem)
    {
        h->mem = old_mem;
        return 1;
    }

    // 检查旧内存是否有效
    if (old_mem)
    {
        memcpy((u8 *)h->mem, (u8 *)old_mem, h->vsize * h->len);
        // 释放旧内存
        free(old_mem);
    }

    return 0;
}

int vec_cap_scaling(vec *h, size cap)
{
    void *old_mem = vec_cap_replace_mem(h, cap);
    if (!h->mem)
    {
        h->mem = old_mem;
        return 1;
    }

    if (old_mem)
    {
        memcpy((u8 *)h->mem, (u8 *)old_mem, h->vsize * h->cap);
        // 释放旧内存
        free(old_mem);
    }

    return 0;
}

int vec_resize(vec *h, size cap)
{
    if (cap < h->cap)
    {
        return vec_cap_scaling(h, cap);
    }

    return vec_cap_expansion(h, cap);
}

/**
 * @brief 计算新的cap
 *
 * @param new_len
 * @param cap
 * @param min_non_zero_cap
 * @return size
 */
size vec_new_cap(size new_len, size cap, size min_non_zero_cap)
{
    size new_cap = (new_len > min_non_zero_cap ? new_len : min_non_zero_cap);
    if ((2 * cap) > new_cap)
    {
        new_cap = 2 * cap;
    }
    return new_cap;
}

size vec_push(vec *h, void *v)
{
    size new_len = h->len + 1;
    if (new_len > h->cap)
    {
        size new_cap = vec_new_cap(new_len, h->cap, h->min_non_zero_cap);
        if (vec_resize(h, new_cap))
        {
            return -1;
        };
    }
    memcpy((u8 *)h->mem + (h->vsize * h->len), v, h->vsize);
    h->len = new_len;

    return new_len - 1;
}

int vec_put(vec *h, void *v, size i)
{
    if (i > h->len)
    {
        return 1;
    }

    size ri = i >= 0 ? i : h->len + i;
    if (ri < 0)
    {
        return 1;
    }

    memcpy((u8 *)h->mem + (ri * h->vsize), v, h->vsize);
    return 0;
}

int vec_insert(vec *h, void *v, size i)
{
    if (i > h->len)
    {
        return 1;
    }

    if (i == h->len)
    {
        vec_push(h, v);
        return 0;
    }

    size ri = i >= 0 ? i : h->len + i;
    if (ri < 0)
    {
        return 1;
    }

    size new_len = h->len + 1;
    if (new_len > h->cap)
    {
        size new_cap = vec_new_cap(new_len, h->cap, h->min_non_zero_cap);
        void *old_mem = vec_cap_replace_mem(h, new_cap - h->cap);
        memcpy((u8 *)h->mem, (u8 *)old_mem, h->vsize * ri);
        memcpy((u8 *)h->mem + (h->vsize * (ri + 1)), (u8 *)old_mem + (h->vsize * ri), h->vsize * (h->len - ri));
        memcpy((u8 *)h->mem + (h->vsize * ri), v, h->vsize);
        free((u8 *)old_mem);
    }
    else
    {
        memmove(
            (u8 *)h->mem + ((ri + 1) * h->vsize),
            (u8 *)h->mem + (ri * h->vsize),
            (h->len - ri) * h->vsize);
        memcpy((u8 *)h->mem + (ri * h->vsize), v, h->vsize);
    }

    h->len = new_len;
    return 0;
}

vec *vec_from(u32 vsize, void *src, u32 len)
{
    if (!src)
    {
        return NULL;
    }

    vec *h = vec_new(vsize);
    size new_cap = vec_new_cap(len, h->cap, h->min_non_zero_cap);
    if (vec_resize(h, new_cap))
    {
        return NULL;
    }

    memcpy((u8 *)h->mem, src, len * vsize);
    h->len = len;

    return h;
}

void *vec_get(vec *h, size i)
{
    if (i > h->len)
    {
        return NULL;
    }

    size ri = i >= 0 ? i : h->len + i;
    if (ri < 0)
    {
        return NULL;
    }

    return (u8 *)h->mem + (ri * h->vsize);
}

void vec_pop(vec *h)
{
    if (h->len == 0)
    {
        return;
    }

    h->len--;
    return;
}

int vec_remove(vec *h, size i)
{
    if (i >= h->len)
    {
        return 1;
    }

    size ri = i >= 0 ? i : h->len + i;
    if (ri < 0)
    {
        return 1;
    }

    h->len--;
    memmove(
        (u8 *)h->mem + (ri * h->vsize),
        (u8 *)h->mem + ((ri + 1) * h->vsize),
        (h->len - ri) * h->vsize);
    return 0;
}

inline void vec_clear(vec *h)
{
    if (!h)
        return;
    h->len = 0;
}

void vec_free(vec *h)
{
    if (h->bind_pp)
    {
        *(u8 **)h->bind_pp = NULL;
    }
    free(h->mem);
    free(h);
    h = NULL;
}

vec *vec_slice(vec *h, usize start, usize end)
{
    if (!h || start > h->len)
    {
        return NULL;
    }

    if (end > h->len)
    {
        end = h->len;
    }

    if (start == end)
    {
        end++;
    }

    size len = end - start;
    size cap = len > h->min_non_zero_cap ? len : h->min_non_zero_cap;
    void *m = malloc(h->vsize * cap);
    memcpy(m, (u8 *)h->mem + (start * h->vsize), h->vsize * len);

    return vec_create(h->vsize, len, cap, NULL, m);
}

vec *vec_clone(vec *h)
{
    if (!h)
    {
        return NULL;
    }
    void *m = malloc(h->vsize * h->cap);
    memcpy(m, h->mem, h->vsize * h->len);
    return vec_create(h->vsize, h->len, h->cap, NULL, m);
}

size vec_find(vec *h, void *v, b32 (*func)(void *, void *))
{
    if (!h || !v || !func)
    {
        return -1;
    }

    for (size i = 0; i < h->len; i++)
    {
        if (func(vec_get(h, i), v))
        {
            return i;
        }
    }

    return -1;
}

size vec_find_last(vec *h, void *v, b32 (*func)(void *, void *))
{
    if (!h || !v || !func)
    {
        return -1;
    }

    for (size i = (h->len - 1); i > 0; i--)
    {
        if (func(vec_get(h, i), v))
        {
            return i;
        }
    }

    return -1;
}

b32 default_comparison(void *a, void *b, size vsize)
{
    if (!a || !b || vsize == 0)
    {
        return 0;
    }

    for (int i = 0; i < vsize; i++)
    {
        if (*((u8 *)a + i) != *((u8 *)b + i))
        {
            return 0;
        }
    }

    return 1;
}

size vec_find_default(vec *h, void *v)
{
    if (!h || !v)
    {
        return -1;
    }

    for (size i = (h->len - 1); i > 0; i--)
    {
        if (default_comparison(vec_get(h, i), v, h->vsize))
        {
            return i;
        }
    }

    return -1;
}

size vec_find_default_last(vec *h, void *v)
{
    if (!h || !v)
    {
        return -1;
    }

    for (size i = (h->len - 1); i > 0; i--)
    {
        if (default_comparison(vec_get(h, i), v, h->vsize))
        {
            return i;
        }
    }

    return -1;
}
