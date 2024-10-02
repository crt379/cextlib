#include "cstring.h"

void copy(u8 *dst, u8 *src, size len);
inline void copy(u8 *dst, u8 *src, size len)
{
    if (len == 0) {
        return;
    }
    // for (size i = 0; i < len; i++) {
    //     dst[i] = src[i];
    // }
    memcpy(dst, src, len);
}

b32 u8s_eq(u8 *u1, u8* u2, size len);

inline b32 u8s_eq(u8 *u1, u8* u2, size len)
{
    for (size i = 0; i < len; i++) {
        if (u1[i] != u2[i]) {
            return 0;
        }
    }
    return 1;
}

size i64_2_str(u8 *s, i64 value) 
{
    u8 *p, aux;
    u64 v;
    usize l;

    /* Generate the string representation, this method produces
     * an reversed string. */
    if (value < 0) {
        /* Since v is unsigned, if value==LLONG_MIN then
         * -LLONG_MIN will overflow. */
        if (value != I64_MIN) {
            v = -value;
        } else {
            v = ((u64)I64_MAX) + 1;
        }
    } else {
        v = value;
    }

    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);
    if (value < 0) *p++ = '-';

    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';

    /* Reverse the string. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

size sunday (const u8 *s1, size s1_len, const u8 *s2, size s2_len)
{
    b8 y = -1;
    size i = 0;
    size j = 0;

    if (s1 == NULL || s2 == NULL || s1_len < s2_len) {
        return -1;
    }

    while (i < s1_len && j < s2_len) {
        if (s1[i] == s2[j]) {
            i++;
            j++; 
        } else {
            // focus on the last one after this character
            size num = i - j + s2_len;
            for (j = s2_len - 1; j >= 0; j--) {
                // the character is compared to the pattern string, 
                // and the same is found to be aligned with the character.
                if (s1[num] == s2[j]) {
                    i = num - j;
                    y = 1;
                    break;
                }
            }
            if (y == -1) {
                // No pattern string is found, jump directly to the next 
                // digit of the character.
                i = num + 1; 
            }
            j = 0;
            y = -1;
        }
    }       

    if (i == s1_len) {
        return -1;
    } 

    return i - s2_len;
}

string* string_new(size cap)
{
    string* s = (string*)malloc(sizeof(string));
    s->buf = NULL;
    if (cap > 0) {
        s->buf = (u8*)malloc(sizeof(u8) * cap);
        if (!s->buf) {
            free(s);
            return NULL;
        }
    }
    s->len = 0;
    s->cap = cap;
    return s;
}

void string_cap_expansion(string *s, size expa_size) 
{
    size new_cap = s->cap + expa_size;
    u8 *new_buf = (u8*)malloc(sizeof(u8) * new_cap);
    copy(new_buf, s->buf, s->len);
    if (!(s->cap == 0 && s->len > 0)) {
        free2(s->buf);
    }
    s->buf = new_buf;
    s->cap = new_cap;
}

void string_push(string *s, u8 *data, size len)
{
    size new_len = s->len + len;
    if (s->cap <= new_len) {
        string_cap_expansion(s, new_len - s->cap + 1);
    }
    copy(s->buf+s->len, data, len);
    s->buf[new_len] = '\0';
    s->len = new_len;
}

void string_push_char(string *s, char c)
{
    size new_len = s->len + 1;
    if (s->cap <= new_len) {
        string_cap_expansion(s, new_len - s->cap + 1);
    }
    s->buf[s->len] = c;
    s->buf[new_len] = '\0';
    s->len = new_len;
}

void string_push_i64(string *s, i64 value)
{
    size new_len = s->len + I64STR_SIZE;
    if (s->cap <= new_len) {
        string_cap_expansion(s, new_len - s->cap + 1);
    }
    size v_len = i64_2_str(s->buf+s->len, value);
    s->len = s->len + v_len;
}

string* string_clone(string *s1) 
{
    string* s2 = string_new(s1->cap);
    copy(s2->buf, s1->buf, s1->len + 1);
    return s2;
}

string* string_from_char(char* c)
{
    size c_len = strlen(c);
    string* s = string_new(c_len + 1);
    string_push(s, (u8*)c, c_len);
    return s;
}

b32 string_slice(string *dst, string *src, size start, size end)
{
    if (!dst || start > src->len) {
        return 1;
    }
    if (end > src->len) {
        end = src->len;
    }
    if (start == end) {
        end++;
    }

    size len = end - start;
    if (dst->cap < (len+1)) {
        free2(dst->buf);
        dst->buf = (u8*)malloc(sizeof(u8) * len+1);
        dst->cap = len+1;
    }
    dst->len = 0;

    string_push(dst, src->buf+start, len);
    return 0;
}

void string_free(string *s)
{
    if (!s) {
        return;
    }
        
    if (s && s->cap > 0) {
        free(s->buf);
    }

    free(s);
}

b32 string_eq(string *s1, string *s2)
{
    if (s1->len != s2->len) {
        return 0;
    }
    
    return u8s_eq(s1->buf, s2->buf, s1->len);
}

b32 string_splice_n(string *dst, ...)
{
    if (!dst) {
        return 1;
    }

    va_list srcs;
    va_start(srcs, dst);

    string *next;
    size new_len = dst->len;
    while ((next = va_arg(srcs, string*))) {
        new_len = new_len + next->len;
    }
    va_end(srcs);
    string_cap_expansion(dst, new_len - dst->cap + 1);
    
    va_start(srcs, dst);
    while ((next = va_arg(srcs, string*))) {
        string_push(dst, next->buf, next->len);
    }
    va_end(srcs);
    
    return 0;
}

u32 string_hash(string *s)
{
    u64 h = 0x100;
    for (size i = 0; i < s->len; i++) {
        h ^= s->buf[i];
        h *= 1111111111111111111u;
    }
    return (h ^ h>>32) & (u32)-1;
}

size string_find(string *s, string *p)
{
    if (!s || !p) {
        return -1;
    }

    return sunday(s->buf, s->len, p->buf, p->len);
}

string_reader reader_new(string *s)
{
    string_reader reader = {
        .str = s,
        .cur = 0,
    };
    return reader;
}

b32 reader_is_end(string_reader *reader)
{
    return reader->cur >= reader->str->len;
}

void reader_reset(string_reader *reader)
{
    reader->cur = 0;
}

b32 reader_skip_next_char(string_reader *reader)
{
    if (!reader_is_end(reader)) {
        reader->cur++;
        return 1;
    }

    return 0;
}

#define u8length(s) u8_length[((u8)s & 0xFF) >> 4];
static u8 const u8_length[] = {
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F
    1,1,1,1,1,1,1,1,0,0,
    0,0,2,2,3,4,
};

static int utf8isvalid(u32 c)
{
    if (c <= 0x7F) 
        return 1;                               // [1]

    if (0xC280 <= c && c <= 0xDFBF)             // [2]
        return ((c & 0xE0C0) == 0xC080);

    if (0xEDA080 <= c && c <= 0xEDBFBF)         // [3]
        return 0; // Reject UTF-16 surrogates

    if (0xE0A080 <= c && c <= 0xEFBFBF)         // [4]
        return ((c & 0xF0C0C0) == 0xE08080);

    if (0xF0908080 <= c && c <= 0xF48FBFBF)     // [5]
        return ((c & 0xF8C0C0C0) == 0xF0808080);

    return 0;
}

u8* reader_get_utf8char(string_reader *reader)
{
    if (!reader || !reader->str || reader->cur >= reader->str->len) {
        return NULL;
    }

    u32 encoding = 0;
    u8* buf = reader->str->buf + reader->cur;
    size len = u8length(buf[0]);
    for (int i = 0; i < len && buf[i] != '\0'; i++) {
        encoding = (encoding << 8) | buf[i];
    }

    if (len == 0 || !utf8isvalid(encoding)) {
        return NULL;
    }

    reader->cur = reader->cur + len;
    u8* ret = (u8*)malloc(len + 1);
    copy(ret, buf, len);
    ret[len] = '\0';
    return ret;
}

u8* reader_get_utf8char_end(string_reader *reader)
{
    u8* c = reader_get_utf8char(reader);
    if (c) {
        return c;
    }

    if (reader && reader->str && reader->cur < reader->str->len){
        u8* ret = (u8*)malloc(2);
        ret[0] = reader->str->buf[reader->cur];
        ret[1] = '\n';
        reader->cur++;
        return ret;
    }

    return NULL;
}

size get_utf8_len(string *s)
{
    u8* c;
    size len = 0;
    string_reader reader = reader_new(s);
    
    do {
        c = reader_get_utf8char(&reader);
        if (c) {
            len++;
            free(c);
        }
    } while (c || reader_skip_next_char(&reader));

    return len;
}