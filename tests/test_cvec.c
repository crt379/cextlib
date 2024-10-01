#include "../cvec.h"
#include <assert.h>
#include <stdio.h>

#define vec_get_tt(h, i) *(tt*)vec_get(h, i)
#define vec_get_int(h, i) *(int*)vec_get(h, i)

typedef struct tt {
    int a;
    float b;
    char c; 
} tt;

void test_new_push()
{
    printf("============== test_new_push ===========\n");
    
    vec* vec1 = vec_new(sizeof(int));
    int** arr1 = (int**)&vec1->mem;
    assert(vec1->len == 0 && vec1->cap == 0);
    assert(vec_push_v(vec1, 1) == 0 && "vec_push_v index faile");
    assert(vec_push_v(vec1, 2) == 1);
    assert(vec_push_v(vec1, 3) == 2);
    assert(vec_push_v(vec1, 4) == 3);
    assert(vec_push_v(vec1, 5) == 4);
    assert(vec_get_int(vec1, -1) == 5 && "vec_push_v failed");
    for (size_t i = 0; i < vec1->len; i++)
    {
        assert((*arr1)[i] == (i+1) && "vec_push_v failed");
    }

    int* arr2 = NULL;
    // vec vec2 = vec_new_bind(&arr2);
    vec* vec2 = vec_new_bind(arr2);
    vec_push_v(vec2, 1);
    vec_push_v(vec2, 2);
    vec_push_v(vec2, 3);
    vec_push_v(vec2, 4);
    assert(arr2[3] == 4 && "vec_push_v failed");
    vec_push_v(vec2, 5);
    assert(arr2[4] == 5 && "vec_push_v failed");
    for (size_t i = 0; i < vec2->len; i++)
    {
        printf("%i", arr2[i]);
    }

    tt *arr3;
    vec* vec3 = vec_new_bind(arr3);

    tt t1 = {1, 1.0, 'a'};
    tt t2 = {2, 2.0, 'b'};
    tt t3 = {3, 3.0, 'c'};
    tt t4 = {4, 4.0, 'd'};
    
    vec_push(vec3, &t1);
    vec_push(vec3, &t2);
    vec_push(vec3, &t3);
    vec_push(vec3, &t4);
    
    tt t5 = vec_get_tt(vec3, 1);
    assert(t5.a == 2 && t5.c == 'b' && "vec_push_v failed");
    printf("\n t5.a: %i, t5.b: %f, t5.c: %c", t5.a, t5.b, t5.c);
}

void test_slice_clone_put()
{
    printf("\n============== test_slice_clone_put ===========\n");
    
    vec* vec1 = vec_new(sizeof(int));
    vec_push_v(vec1, 1);
    vec_push_v(vec1, 2);
    vec_push_v(vec1, 3);
    vec_push_v(vec1, 4);
    vec_push_v(vec1, 5);

    vec* vec2 = vec_clone(vec1);
    assert(vec1->mem != vec2->mem && "vec_clone failed");
    assert(vec_get_int(vec2, -1) == 5 && "vec_clone failed");

    vec_put_v(vec2, 6, -1);
    assert(vec_get_int(vec2, -1) == 6 && "vec_put failed");
    assert(vec_get_int(vec1, -1) == 5 && "vec_clone failed");

    vec* vec3 = vec_slice(vec1, 1, 3);
    assert(vec3->len == 2 && "vec_slice failed");
    assert(vec_get_int(vec3, -1) == 3 && "vec_slice failed");
}

void test_pop()
{
    printf("\n============== test_pop ===========\n");
    
    vec* vec1 = vec_new(sizeof(int));
    vec_push_v(vec1, 1);
    vec_push_v(vec1, 2);
    vec_push_v(vec1, 3);
    vec_push_v(vec1, 4);
    vec_push_v(vec1, 5);
    vec_pop(vec1);
    assert(vec_get_int(vec1, -1) == 4 && "vec_pop failed");
    int* arr1 = (int*)vec1->mem;
    for (size_t i = 0; i < vec1->len; i++)
    {
        printf("%i", arr1[i]);
    }
}

void test_remove()
{
    printf("\n============== test_remove ===========\n");
    vec* vec1 = vec_new(sizeof(int));
    vec_push_v(vec1, 1);
    vec_push_v(vec1, 2);
    vec_push_v(vec1, 3);
    vec_push_v(vec1, 4);
    vec_push_v(vec1, 5);
    vec_remove(vec1, 1);
    assert(vec_get_int(vec1, 1) == 3 && "vec_remove failed");
    int* arr1 = (int*)vec1->mem;
    for (size_t i = 0; i < vec1->len; i++)
    {
        printf("%i", arr1[i]);
    }
}

void test_insert()
{
    printf("\n============== test_insert ===========\n");
    vec* vec1 = vec_new(sizeof(int));
    vec_push_v(vec1, 1);
    vec_push_v(vec1, 2);
    vec_push_v(vec1, 3);
    vec_push_v(vec1, 4);
    vec_insert_v(vec1, 6, 2);
    vec_push_v(vec1, 5);
    // assert(vec_get_int(vec1, 2) == 6 && "vec_insert failed");
    // vec_insert_v(vec1, 6, 2);
    int* arr1 = (int*)vec1->mem;
    for (size_t i = 0; i < vec1->len; i++)
    {
        printf("%i", arr1[i]);
    }
}

void test_find()
{
    printf("\n============== test_find ===========\n");
    vec* vec1 = vec_new(sizeof(int));
    vec_push_v(vec1, 1);
    vec_push_v(vec1, 2);
    vec_push_v(vec1, 3);
    vec_push_v(vec1, 4);
    vec_push_v(vec1, 5);
    assert(vec_find_default_v(vec1, 2) == 1 && "vec_find failed");
    assert(vec_find_default_v(vec1, 5) == 4 && "vec_find failed");
}

int main()
{
    test_new_push();
    test_slice_clone_put();
    test_pop();
    test_remove();
    test_insert();
    test_find();
    
    return 0;
}