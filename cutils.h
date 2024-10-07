#include <stdio.h>

#define MALLOC_CHECK(p, s, f)         \
    {                                 \
        p = (__typeof__(p))malloc(s); \
        if (p == NULL)                \
        {                             \
            f;                        \
            return NULL;              \
        }                             \
    }

#define CMALLOC_CHECK(p, l, s, f)        \
    {                                    \
        p = (__typeof__(p))calloc(l, s); \
        if (p == NULL)                   \
        {                                \
            f;                           \
            return NULL;                 \
        }                                \
    }

#define MALLOC_CHECK_COND_NULL(p, s, f, c) \
    {                                      \
        if (c)                             \
        {                                  \
            p = NULL;                      \
        }                                  \
        else                               \
        {                                  \
            MALLOC_CHECK(p, s, f);         \
        }                                  \
    }

#define CMALLOC_CHECK_COND_NULL(p, l, s, f, c) \
    {                                          \
        if (c)                                 \
        {                                      \
            p = NULL;                          \
        }                                      \
        else                                   \
        {                                      \
            CMALLOC_CHECK(p, l, s, f);         \
        }                                      \
    }

#define VALUE_SWAP(a, b)       \
    {                          \
        __typeof__(a) tmp = a; \
        a = b;                 \
        b = tmp;               \
    }