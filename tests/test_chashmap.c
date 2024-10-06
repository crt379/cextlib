#include "../chashmap.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

void hashmap_print(hashmap *map)
{
    if (!map)
    {
        return;
    }
    printf("map(%p): \n", map);
    printf("    buckets: %p\n", map->buckets);
    printf("    keys: %p\n", map->keys);
    printf("    values: %p\n", map->values);
    printf("    values_flags: %p\n", map->values_flags);
    printf("    cap: %zu\n", map->cap);
    printf("    len: %zu\n", map->len);
    printf("    resize_len: %zu\n", map->resize);
    printf("    ksize: %zu\n", map->ksize);
    printf("    vsize: %zu\n", map->vsize);
    printf("    kdsize: %zu\n", map->kdsize);
    printf("    vdsize: %zu\n", map->vdsize);
    printf("    seed: %zu\n", map->seed);
    printf("    keys_swap: %p\n", map->keys_swap);
    printf("    values_swap: %p\n", map->values_swap);
    printf("    hasher: %p\n", map->hasher);
    printf("    cmp: %p\n", map->cmp);
}

void print_int_map_keys(hashmap *map)
{
    int *key;
    hashmap_iterator iter = hashmap_begin(map);

    printf("============== print_int_map_keys(%p) ===========\n", map);
    while (!hashmap_iter_is_end(&iter))
    {
        key = hashmap_iter_key(&iter);
        printf("key: %i\n", *key);
    }
}

void print_int_map_values(hashmap *map)
{
    int *value;
    hashmap_iterator iter = hashmap_begin(map);
    printf("============== print_int_map_values(%p) ===========\n", map);
    while (!hashmap_iter_is_end(&iter))
    {
        value = hashmap_iter_value(&iter);
        printf("value: %i\n", *value);
    }
}

void print_int_map_items(hashmap *map)
{
    hashmap_iterator_kv kv;
    hashmap_iterator iter = hashmap_begin(map);
    printf("============== print_int_map_items(%p) ===========\n", map);
    while (!hashmap_iter_is_end(&iter))
    {
        kv = hashmap_iter_kv(&iter);
        printf("key: %i, value: %i\n", *(int *)kv.key, *(int *)kv.value);
    }
}

void test_new()
{
    printf("============== test_new ===========\n");
    hashmap *map;

    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    assert(map != NULL);
    assert(map->buckets != NULL);
    assert(map->keys != NULL);
    assert(map->values != NULL);
    assert(map->cap == INITIAL_BUCKETS);
    assert(map->len == 0);
    assert(map->hasher);
    hashmap_print(map);
    hashmap_free(map);

    map = hashmap_new(sizeof(int), sizeof(hashmap), 123456, NULL, NULL);
    hashmap_print(map);
    hashmap_free(map);
}

void test_set_and_get()
{
    printf("============== test_set ===========\n");
    hashmap *map;

    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    for (int item = 0; item < 20; item++)
    {
        // printf("item: %i, map len: %td, map rsize: %td\n", item, map->len, map->resize);
        hashmap_set(map, &item, &item);
    }
    assert(map->len == 20);

    for (int item = 10; item < 30; item++)
    {
        hashmap_set(map, &item, &item);
    }
    assert(map->len == 30);

    for (int item = 10; item < 30; item++)
    {
        hashmap_set(map, &item, &item);
    }
    assert(map->len == 30);

    // put
    assert(*(int *)hashmap_get(map, &(int){1}) == 1);
    hashmap_set(map, &(int){1}, &(int){2});
    assert(*(int *)hashmap_get(map, &(int){1}) == 2);
    hashmap_free(map);

    // NULL value
    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    hashmap_set(map, &(int){1}, NULL);
    assert(map->len == 1);
    assert(hashmap_get(map, &(int){1}) == NULL);
    assert(hashmap_exist(map, &(int){1}));

    hashmap_set(map, &(int){2}, NULL);
    assert(map->len == 2);
    assert(hashmap_get(map, &(int){2}) == NULL);
    assert(hashmap_exist(map, &(int){2}));
    hashmap_free(map);

    // zero value
    map = hashmap_new(sizeof(int), 0, 123456, NULL, NULL);
    assert(map->values != NULL);
    assert(map->values_swap != NULL);

    hashmap_set(map, &(int){1}, NULL);
    assert(map->len == 1);

    hashmap_set(map, &(int){2}, NULL);
    assert(map->len == 2);
    
    assert(hashmap_get(map, &(int){2}) == NULL);
    assert(hashmap_exist(map, &(int){2}));
    hashmap_free(map);

    // NULL key
    map = hashmap_new(sizeof(int), 0, 123456, NULL, NULL);
    hashmap_set(map, NULL, &(int){1});
    assert(map->len == 1);
    assert(*(int *)hashmap_get(map, NULL) == 1);
    assert(hashmap_exist(map, NULL));

    hashmap_set(map, NULL, &(int){2});
    assert(*(int *)hashmap_get(map, NULL) == 2);
    
    hashmap_free(map);
}

void test_iteration()
{
    printf("============== test_iteration ===========\n");
    hashmap *map;

    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    for (int item = 0; item < 20; item++)
    {
        hashmap_set(map, &item, &item);
    }
    print_int_map_keys(map);

    print_int_map_values(map);

    print_int_map_items(map);

    hashmap_free(map);
}

void test_del()
{
    printf("============== test_del ===========\n");
    hashmap *map;

    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    for (int item = 0; item < 20; item++)
    {
        hashmap_set(map, &item, &item);
    }
    assert(map->len == 20);

    hashmap_remove(map, &(int){1});
    assert(map->len == 19);

    hashmap_remove(map, &(int){17});
    assert(map->len == 18);

    hashmap_remove(map, &(int){3});
    assert(map->len == 17);

    print_int_map_keys(map);
    hashmap_free(map);
}

void test_update()
{
    printf("============== test_update ===========\n");
    hashmap *map;
    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    hashmap_set(map, &(int){1}, &(int){1});
    hashmap_set(map, &(int){2}, &(int){2});
    hashmap_set(map, &(int){3}, &(int){3});

    hashmap *map2 = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    hashmap_set(map2, &(int){1}, &(int){11});
    hashmap_set(map2, &(int){2}, &(int){22});

    hashmap_update(map, map2);
    print_int_map_items(map);

    hashmap_free(map);
    hashmap_free(map2);
}

void test_clone()
{
    printf("============== test_clone ===========\n");
    hashmap *map;
    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    hashmap_set(map, &(int){1}, &(int){1});
    hashmap_set(map, &(int){2}, &(int){2});
    hashmap_set(map, &(int){3}, &(int){3});

    hashmap *map2 = hashmap_clone(map);
    hashmap_set(map2, &(int){4}, &(int){5});

    print_int_map_items(map);
    print_int_map_items(map2);

    hashmap_free(map);
    hashmap_free(map2);
}

void test_free()
{
    printf("============== test_free ===========\n");
    hashmap *map;
    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    hashmap_set(map, &(int){1}, &(int){1});
    hashmap_set(map, &(int){2}, &(int){2});
    hashmap_set(map, &(int){3}, &(int){3});

    hashmap_free(map);
    hashmap_print(map);
    // hashmap_set(map, &(int){4}, &(int){4});
}

int main()
{
    printf("============== START ===========\n");
    test_new();
    test_set_and_get();
    test_iteration();
    test_del();
    test_update();
    test_clone();
    test_free();
    printf("============== DONE ===========\n");
    return 0;
}