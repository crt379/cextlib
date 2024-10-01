#ifndef __CTYPE_H
#define __CTYPE_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef uintptr_t uptr;
typedef size_t    usize;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef ptrdiff_t size;
typedef char      byte;

typedef int8_t    b8;
typedef int32_t   b32;

#define sizeof(x)    (size)sizeof(x)
#define alignof(x)   (size)_Alignof(x)
#define countof(a)   (sizeof(a)/sizeof(*(a)))
#define lengthof(s)  (countof(s) - 1)

#define free2(ptr) if (ptr) free(ptr)

#endif // __CTYPE_H