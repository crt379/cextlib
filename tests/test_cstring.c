#include "../cstring.h"
#include <stdio.h>

void test_push()
{
    printf("============== test_push ===========\n");
    string *s1 = string_new(0);
    string_push(s1, (u8 *)"1234567890", 10);
    printf("%ti, %s\n", s1->cap, s1->buf);
    string_free(s1);

    string *s3 = string_new(0);
    string_push_i64(s3, 7896541230);
    printf("%ti, %s\n", s3->cap, s3->buf);
    string_free(s3);
}

void test_from()
{
    printf("============== test_from ===========\n");
    string *s1 = string_from_char("1234567890");
    printf("%ti, %s\n", s1->cap, s1->buf);
    string_free(s1);

    string *s2 = string_from_char("你好，世界！123 \x82 ffff");
    printf("%ti, %s\n", s2->cap, s2->buf);
    string_free(s2);
}

void test_clone()
{
    printf("============== test_clone ===========\n");
    string *s1 = string_new(10);
    string_push(s1, (u8 *)"1234567890", 8);
    string *s2 = string_clone(s1);
    printf("%s - %s\n", s1->buf, s2->buf);
    string_free(s2);
}

void test_slice()
{
    printf("============== test_slice ===========\n");
    string *s1 = string_new(10);
    string_push(s1, (u8 *)"1234567890", 8);

    string *s2 = string_new(10);
    b32 ret = string_slice(s2, s1, 3, 8);

    printf("%i, %s - %s\n", ret, s1->buf, s2->buf);
    string_free(s1);
    string_free(s2);
}

void test_eq()
{
    printf("============== test_eq ===========\n");
    string *s1 = string_new(10);
    string_push(s1, (u8 *)"1234567890", 8);

    string *s2 = string_new(10);
    string_push(s2, (u8 *)"abcdefghij", 8);
    printf("%s eq %s r:%i\n", s1->buf, s2->buf, string_eq(s1, s2));

    string *s3 = string_new(10);
    string_push(s3, (u8 *)"1234567890", 8);
    printf("%s eq %s r:%i\n", s1->buf, s3->buf, string_eq(s1, s3));

    string_free(s1);
    string_free(s2);
    string_free(s3);
}

void test_splice()
{
    printf("============== test_splice ===========\n");
    string *s1 = string_new(10);
    string_push(s1, (u8 *)"1234567890", 8);

    string *s2 = string_new(10);
    string_push(s2, (u8 *)"abcdefghij", 8);

    b32 ret = string_splice(s1, s2);
    printf("%s r:%i\n", s1->buf, ret);

    string_free(s1);
    string_free(s2);
}

void test_utf8()
{
    printf("============== test_utf8 ===========\n");
    string *s1 = string_from_char("你好，世界！123 \x82 ffff");
    printf("%ti, %s\n", s1->cap, s1->buf);

    u8 *u8chr;
    string_reader reader = reader_new(s1);

    do
    {
        u8chr = reader_get_utf8char(&reader);
        if (u8chr)
        {
            printf("%s\n", u8chr);
            free(u8chr);
        }
    } while (!reader_is_end(&reader) && u8chr);
    printf("done 1\n");

    reader_reset(&reader);
    do
    {
        u8chr = reader_get_utf8char_end(&reader);
        if (u8chr)
        {
            printf("%s\n", u8chr);
            free(u8chr);
        }
    } while (u8chr);
    printf("done 2\n");

    size utf8len = get_utf8_len(s1);
    printf("utf8 char len: %ti\n", utf8len);

    string_free(s1);
}

void test_find()
{
    string *s1 = string_from_char("你好，世界！123 \x82 ffff");
    string *p1 = string_from_char("你好，世界！");
    string *p2 = string_from_char("123");
    string *p3 = string_from_char("ddd");

    printf("s1 find p1, i: %ti\n", string_find(s1, p1));
    printf("s1 find p2, i: %ti\n", string_find(s1, p2));
    printf("s1 find p3, i: %ti\n", string_find(s1, p3));

    string_free(s1);
    string_free(p1);
    string_free(p2);
    string_free(p3);
}

int main()
{
    test_push();
    test_from();
    test_clone();
    test_slice();
    test_eq();
    test_splice();
    test_utf8();
    test_find();
    return 0;
}