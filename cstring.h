#ifndef __CSTRING_H
#define __CSTRING_H

#include "ctype.h"
#include <assert.h>
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>


#define I64_MIN     LLONG_MIN
#define I64_MAX     LLONG_MAX
#define I64STR_SIZE 21

/*
 * @brief i64转字符串
 *
 * @param s
 * @param value
 * @return size
 */
size i64_2_str(u8 *s, i64 value);

/*
 * @brief sunday算法寻找模式串在主串中第一次出现的位置
 *
 * @param s1
 * @param s1_len
 * @param s2
 * @param s2_len
 * @return size
 */
size sunday(const u8 *s1, size s1_len, const u8 *s2, size s2_len);

// ============================================================================
// string
// ============================================================================

typedef struct string
{
    u8 *buf;
    size len;
    size cap;
} string;

/*
 * @brief 创建一个字符串
 *
 * @param string*
 */
string *string_new(size cap);

/*
 * @brief 扩容字符串
 *
 * @param string*
 * @param expa_size
 */
void string_cap_expansion(string *s, size expa_size);

/*
 * @brief 向字符串末尾追加数据
 *
 * @param string*
 * @param data
 * @param len
 */
void string_push(string *s, u8 *data, size len);

/*
 * @brief 向字符串末尾追加字符
 *
 * @param string*
 * @param char
 */
void string_push_char(string *s, char c);

/*
 * @brief 向字符串末尾追加i64
 *
 * @param string*
 * @param i64
 */
void string_push_i64(string *s, i64 value);

/*
 * @brief 复制字符串
 *
 * @param string*
 * @return string*
 */
string *string_clone(string *s1);

/*
 * @brief 从char字符串创建string字符串
 *
 * @param u8*
 * @param len
 * @param cap
 * @return string*
 */
string *string_from_char(char *c);

/*
 * @brief 从字符串s1中截取s2到s3之间的字符串
 *
 * @param string*
 * @param string*
 * @param string*
 * @return b32
 */
b32 string_slice(string *dst, string *src, size start, size end);

/*
 * @brief 释放字符串
 *
 * @param string*
 */
void string_free(string *s);

/*
 * @brief 比较两个字符串是否相等
 *
 * @param string*
 * @param string*
 * @return b32
 */
b32 string_eq(string *s1, string *s2);

#define string_splice(s, ...) string_splice_n(s, __VA_ARGS__, NULL)
/*
 * @brief 在字符串末尾追加多个字符串, 要以NULL为结尾
 *
 * @param string* dst
 * @param ...
 */
b32 string_splice_n(string *dst, ...);

/*
 * 查找子串位置
 *
 * @param string*
 * @param string*
 */
size string_find(string *s, string *p);

// ============================================================================
// reader
// ============================================================================

typedef struct _string_reader
{
    string *str;
    size cur;
} string_reader;

/*
 * @brief 创建一个string_reader
 *
 * @param string*
 */
string_reader reader_new(string *s);

/*
 * @brief 判断是否已经读完
 */
b32 reader_is_end(string_reader *reader);

/*
 * @brief 重置string_reader
 */
void reader_reset(string_reader *reader);

/*
 * @brief 跳过下一个字符
 */
b32 reader_skip_next_char(string_reader *reader);

/*
 * @brief 获取下一个utf8字符
 */
u8 *reader_get_utf8char(string_reader *reader);

/*
 * @brief 获取下一个utf8字符, 不是utf8也读取
 */
u8 *reader_get_utf8char_end(string_reader *reader);

/*
 * @brief 获取utf8字符串长度
 */
size get_utf8_len(string *s);

// ============================================================================
// utf8 codec
// ============================================================================

// from UTF-8 encoding to Unicode Codepoint
static u32 utf8decode(u32 c)
{
    u32 mask;

    if (c > 0x7F)
    {
        mask = (c <= 0x00EFBFBF) ? 0x000F0000 : 0x003F0000;
        c = ((c & 0x07000000) >> 6) |
            ((c & mask) >> 4) |
            ((c & 0x00003F00) >> 2) |
            (c & 0x0000003F);
    }

    return c;
}

// From Unicode Codepoint to UTF-8 encoding
static u32 u8encode(u32 codepoint)
{
    u32 c = codepoint;

    if (codepoint > 0x7F)
    {
        c = (codepoint & 0x000003F) |
            (codepoint & 0x0000FC0) << 2 |
            (codepoint & 0x003F000) << 4 |
            (codepoint & 0x01C0000) << 6;

        if (codepoint < 0x0000800)
            c |= 0x0000C080;
        else if (codepoint < 0x0010000)
            c |= 0x00E08080;
        else
            c |= 0xF0808080;
    }
    return c;
}

#endif // __CSTRING_H