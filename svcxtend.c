#include <stdio.h>
#include <assert.h>

#define SVCX_IMPLEMENTATION
#include "svcxtend.h"

int main() {
    /*svcx_arena arena;
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
      printf("\n");*/

    svcx_arena arena;
    svcx_arena_init(&arena, 1024 * 1024);
    svcx_allocator alloc = svcx_arena_allocator(&arena);

    svcx_string_view sv1 = svcx_sv_from_cstr("hello world");
    svcx_string_view sv2 = svcx_sv_from_cstr("hello");
    svcx_string_view sv3 = svcx_sv_from_cstr("world");

    assert(svcx_sv_starts_with(sv1, sv2));
    assert(svcx_sv_ends_with(sv1, sv3));
    assert(!svcx_sv_starts_with(sv1, sv3));
    assert(!svcx_sv_ends_with(sv1, sv2));

    svcx_string_view sub = svcx_sv_from_parts(sv1.data + 6, 5);
    assert(svcx_sv_ends_with(sub, svcx_sv_from_cstr("world")));

    svcx_string_builder sb;
    svcx_sb_init(&sb, alloc);

    SVCX_SB_APPEND_LIT(&sb, "Hello");
    SVCX_SB_APPEND_LIT(&sb, ", ");
    SVCX_SB_APPEND_LIT(&sb, "World");

    svcx_string_view exclaim = svcx_sv_from_cstr("!");
    svcx_sb_append_sv(&sb, exclaim);

    svcx_string_view built = svcx_sb_view(&sb);
    assert(built.len == sizeof("Hello, World!") - 1);

    svcx_string_view hello = svcx_sv_from_cstr("Hello");
    svcx_string_view world = svcx_sv_from_cstr("World");

    assert(svcx_sv_contains(built, hello));
    assert(svcx_sv_contains(built, world));
    
    printf("Hello, World!\n");
    return 0;
}
