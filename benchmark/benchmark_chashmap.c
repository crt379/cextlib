#include "../chashmap.h"
#include <time.h>

#define CAP 1000000
#define TEST_SIZE 500000

void benchmark_chashmap_set()
{
    hashmap* map;
    clock_t start_t, end_t;
    
    map = hashmap_new(sizeof(int), sizeof(int), 123456, NULL, NULL);
    start_t = clock();
    for (size i = 0; i < TEST_SIZE; i++) {
        hashmap_set(map, &i, &i);
    }
    end_t = clock();
    printf("cap in %i set %i th time consuming: %fs\n", CAP, TEST_SIZE, (double)(end_t - start_t) / CLOCKS_PER_SEC);
    hashmap_free(map);

    map = hashmap_new_with_cap(CAP,sizeof(int), sizeof(int), 123456, NULL, NULL);
    start_t = clock();
    for (size i = 0; i < TEST_SIZE; i++) {
        hashmap_set(map, &i, &i);
    }
    end_t = clock();
    printf("cap in %i set %i th time consuming: %fs\n", CAP, TEST_SIZE, (double)(end_t - start_t) / CLOCKS_PER_SEC);
    hashmap_free(map);
}

int main()
{
    benchmark_chashmap_set();
    return 0;
}