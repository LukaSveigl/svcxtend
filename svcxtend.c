#include <stdio.h>

#define SVCX_IMPLEMENTATION
#include "svcxtend.h"

int main() {
    svcx_arena arena;
    svcx_arena_init(&arena, 1024 * 1024);

    svcx_vector v;
    svcx_vector_init(&v, sizeof(int), svcx_arena_allocator(&arena));
    //svcx_vector_init(&v, sizeof(int), svcx_default_allocator());
    
    SVCX_VECTOR_PUSH(&v, int, 42);
    SVCX_VECTOR_PUSH(&v, int, 37);
    SVCX_VECTOR_PUSH(&v, int, 12);
    SVCX_VECTOR_PUSH(&v, int, 11);

    svcx_vector_pop(&v, NULL);

    printf("Ret: %d\n",  *(int*)svcx_vector_at(&v, 2));

    int tmp = 69;
    svcx_vector_insert(&v, 2, &tmp);

    printf("Ret: %d\n",  *(int*)svcx_vector_at(&v, 2));

    int tmp_arr[] = {0, 1, 2, 3};
    svcx_vector v2;
    svcx_vector_from_array(&v2, &tmp_arr, SVCX_ARRAY_LEN(tmp_arr), sizeof(int),
                           svcx_default_allocator());

    printf("V2: %d\n", *(int *)svcx_vector_at(&v2, 1));

    foreach_v(int *it, v2) { printf("%d ", *it); }
    printf("\n");

    foreach_a(int *it, tmp_arr) { printf("%d ", *it); }
    printf("\n");
    
    printf("Hello, World!\n");
    return 0;
}
