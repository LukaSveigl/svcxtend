//
// svcxtend - Sveigl's C eXtended
//
// This is a small library containing various utility functions for C
// development.
//
// The structure of this library is inspired by the following projects:
// - https://github.com/tsoding/nob.h
// - https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// # Small example:
//
// ```c
// #define SVCX_IMPLEMENTATION
// #include "svcxtend.h"
//
// int main () {
//     svcx_allocator a = svcx_default_allocator();
//     int *ptr = svcx_alloc(&a, 10 * sizeof(10));
//     ...
// }
// ```
// # Macro interface
//
// This library exposes multiple macros that can be defined by the user
// to change various aspects of the functionality. The list of macros:
//
// - SVCX_IMPLEMENTATION - Enables the definition of functions, if not
//   defined as in the example, only function declarations are included.
// - SVCXDEF - Can be redefined to append additional flags to function
//   declarations, for example `#define SVCXDEF static inline` will
//   append the static inline keywords to all functions in the file.
// - SVCX_DEBUG - If defined, enables assertions inside svcx functions
//   that validate the data passed into them.
//
// # Contents
//
// This library contains various utilities for C development, such as:
// - A default allocator
// - An arena allocator
// - A vector
// - A string view (non-owning) and string builder (owning) constructs


#ifndef SVCXTEND_H
#define SVCXTEND_H

#ifndef SVCXDEF
/*
 * Goes before declarations and definitions of functions in this library. Allows
 * the user to redefine the function prefixes, for example:
 *     `#define SVCXDEF static inline`
 * which allows the user to use the library multiple times in the project
 * without collision. 
 */
#define SVCXDEF
#endif // SVCXDEF

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef SVCX_ASSERT
#ifdef SVCX_DEBUG
#define SVCX_ASSERT(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "%s:%d: ASSERT FAILED: %s", __FILE__, __LINE__, #cond);  \
      abort();                                                                 \
    }                                                                          \
  } while (0)
#else
#define SVCX_ASSERT(cond) ((void)0)
#endif // SVCX_DEBUG
#endif // SVCX_ASSERT

//
// Some helper macros to make code more ergonomic.
//
// The SVCX_UNUSED macro can be used in functions to suppress unused argument
// warnings:
// ```c
// int foo(int unused) {
//     SVCX_UNUSED(unused);
//     return 0;
// }
// ```
//
// The SVCX_UNSUPPORTED macro can be used within functions that have to exist to
// satisfy the compiler but should not be used.
//
// The SVCX_ARRAY_LEN macro is here to make getting array length less cluttered.
//
#define SVCX_UNUSED(value) (void)(value)
#define SVCX_UNSUPPORTED(message) do { fprintf(stderr, "%s:%d: UNSUPPORTED: %s\n", __FILE__, __LINE__, message); abort(); } while (0)
#define SVCX_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))


/*
 * The results returned by SVCX functions.
 *
 * The svcx_error_string function can be used to display the results in human
 * readable form:
 * ```c
 * svcx_result res = svcx_function();
 * if (res != SVCX_OK) {
 *     printf("%s\n", svcx_error_string(res));
 * }
 * ```
 */
typedef enum svcx_result {
    SVCX_OK = 0,
    SVCX_VEC_GROW_MEM_ERR,
    SVCX_VEC_ERR_OOM,
    SVCX_VEC_PUSH_GROW_ERR,
    SVCX_VEC_POP_EMPTY_ERR,
    SVCX_VEC_INSERT_OOB,
    SVCX_VEC_INSERT_GROW_ERR,
    SVCX_VEC_APPEND_STRIDE_ERR,
    SVCX_VEC_APPEND_GROW_ERR,
    SVCX_VEC_FROM_ARR_MALLOC_ERR,
    SVCX_SV_SPLIT_ERR,
    SVCX_SB_PUSHC_ERR,
    SVCX_SB_APPEND_ERR,
    SVCX_SB_APPEND_SV_ERR,
    SVCX_SB_FMT_INVALID_ARG_ERR,
    SVCX_SB_FMT_RESERVE_ERR
} svcx_result;

SVCXDEF const char *svcx_error_string(svcx_result);


typedef void *(*svcx_alloc_fn)(void *ctx, size_t size);
typedef void *(*svcx_realloc_fn)(void *ctx, void *ptr, size_t size);
typedef void (*svcx_free_fn)(void *ctx, void *ptr);

/*
 * The allocator that handles memory access of the library in
 * an attempt to prevent various memory leaks and such problems.
 *
 * Each custom allocator must implement the three listed functions:
 * - allocation
 * - reallocation
 * - freeing
 * Some of these functions can be unsupported or no-ops, like the realloc
 * and free functions in the provided arena allocator, respectively.
 *
 * The ctx (context) member is a member that stores data of more advanced
 * allocators, such as the arena allocator.
 *
 * For an example of how to define a custom allocator, look at the arena
 * allocator code.
 */
typedef struct svcx_allocator {
    svcx_alloc_fn alloc;
    svcx_realloc_fn realloc;  
    svcx_free_fn free;
    void *ctx;    
} svcx_allocator;

//
// Internal default allocator functions. These can be safely ignored.
// To interact with the allocators, use the functions in the block
// beneath this one.
//
SVCXDEF void *svcx__malloc(void *ctx, size_t size);
SVCXDEF void *svcx__realloc(void *ctx, void *ptr, size_t size);
SVCXDEF void svcx__free(void *ctx, void *ptr);


//
// The allocator helper functions, to be used in actual code.
//
// The svcx_default_allocator returns the default allocator, which
// supports all three key operations: allocation, reallocation and
// freeing.
//
// The svcx_alloc function performs the allocator's allocation operation
// and returns a pointer to the allocated memory.
//
// The svcx_realloc function performs the reallocation operation
// of the allocator, if one is available (for example, the arena
// allocator does not support reallocation.
//
// The svcx_free function performs the free operation on the provided
// pointer, if the allocator supports it (for example, in the arena
// allocator, this operation is a no-op and does nothing).
//
// The svcx_allocator_is_valid function checks if the custom defined
// allocator is valid.
//
// The svcx_alloc_zero function uses the allocation function of the
// provided allocator, while also setting the allocated memory to
// zero.
//
// Example:
// ```c
// svcx_allocator a = svcx_default_allocator();
// int *ptr = (int *)svcx_alloc(&a, 10 * sizeof(int));
// ptr = (int *)svcx_realloc(&a, 20 * sizeof(int));
// svcx_free(&a, ptr);
// ```
//
SVCXDEF svcx_allocator svcx_default_allocator(void);
SVCXDEF void *svcx_alloc(svcx_allocator *a, size_t size);
SVCXDEF void *svcx_realloc(svcx_allocator *a, void *ptr, size_t size);
SVCXDEF void svcx_free(svcx_allocator *a, void *ptr);
SVCXDEF bool svcx_allocator_is_valid(const svcx_allocator *a);
SVCXDEF void *svcx_alloc_zero(svcx_allocator *a, size_t size);




/*
 * The arena which is used in the svcx_allocator. It should be used
 * in cases where a lot of memory needs to be allocated and freed in
 * one go.
 *
 * The memory of the arena is stored in bytes and is always aligned to
 * 8 bytes. Once the used size in the arena reaches the capacity, a
 * NULL pointer is returned.
 *
 * The arena allocator only supports the allocation function, as
 * reallocation is an unsupported operation, while freeing individual
 * blocks of memory is a no-op. To clear an arena, use either the
 * svcx_arena_reset or svcx_arena_free function.
 */
typedef struct svcx_arena {
    unsigned char *base;
    size_t size;
    size_t used;
} svcx_arena;

//
// Internal arena allocator functions. These can be safely ignored.
// To create the arena and free it, use the functions in the block
// below.
//
SVCXDEF void *svcx_arena_alloc(void *ctx, size_t size);
SVCXDEF void *svcx_arena_realloc(void *ctx, void *ptr, size_t size);
SVCXDEF void svcx_arena_free(void *ctx, void *ptr);


//
// The arena helper functions, to be used in actual code.
//
// The svcx_arena_allocator returns an arena allocator, which
// holds the provided arena and uses it for memory allocation.
//
// The svcx_arena_init function performs the initialization of
// the arena, which can be passed into the allocator.
//
// The svcx_arena_reset function resets the arena's used counter
// effectively clearing the data an starting the process of
// overwriting it.
//
// The svcx_arena_free_all function frees the arena's memory
// and reseting it's fields, completely zeroing it out.
//
// Example:
// ```c
// svcx_arena arena = {0};
// svcx_arena_init(&a, 1024 * 1024);
// svcx_allocator a = svcx_arena_allocator(&arena);
// int *ptr = svcx_alloc(&a, 1024);
// if (!ptr) {
//     printf("Arena memory depleted\n");
// }
// svcx_arena_reset(&arena);
// int *ptr = svcx_alloc(&a, 1024);
// svcx_arena_Free_all(&arena);
// // Arena invalid here, must be reinitialized
// ```
//
SVCXDEF svcx_allocator svcx_arena_allocator(svcx_arena *arena);
SVCXDEF void svcx_arena_init(svcx_arena *arena, size_t size);
SVCXDEF void svcx_arena_reset(svcx_arena *arena);
SVCXDEF void svcx_arena_free_all(svcx_arena *arena);




/*
 * The vector is essentially a dynamic array that holds data of
 * arbitrary types. The data is stored sequentially in memory,
 * making random access efficient.
 *
 * It uses it's own allocator to handle memory, and is designed
 * to work with any allocator that supports the svcx_alloc and
 * svcx_free functions (svcx_realloc is not necessary).
 */
typedef struct svcx_vector {
    void *data;
    size_t size;
    size_t cap;
    size_t stride;
    svcx_allocator a;
} svcx_vector;

/*
 * An internal vector grow operation. Should not be used by user
 * code as it may cause undefined behavior. To interact with the
 * vector, use the functions in the block below.
 */
SVCXDEF svcx_result _svcx_vector_grow(svcx_vector *v, size_t min_cap);


//
// Functions for manipulating a vector. Some of these functions might be
// slightly unergonomic to use, and ways to use them effectively are
// shown in the example below.
//
// The svcx_vector_init function initializes the given vector with the provided
// allocator. Stride needs to be provided in order to enable the vector to work
// with any type, and is essentially just the size of the data type in the
// vector.
//
// The svcx_vector_clear function resets the vector's size count to 0,
// effectively discarding the data and starting the overwriting process.
//
// The svcx_vector_reserve function attempts to reserve at least the min_cap
// memory in the vector's internal buffer.
//
// The svcx_vector_push function takes in a pointer to the element to be
// added to the end of the vector. Because this can be cumbersome, a macro
// SVCX_VECTOR_PUSH(v, T, value) is provided, allowing the user to pass in
// actual values.
//
// The svcx_vector_pop function removes the last element from the vector and
// returns it in the out_elem pointer.
//
// The svcx_vector_at function returns a pointer to the element at the given
// index.
//
// The svcx_vector_insert function inserts the given element at the provided
// index. Same as the push function, this function accepts a pointer to the
// element to enable the vector to work with any type, however no wrapper
// macro is provided for this function, as this operation is usually not
// that common.
//
// The svcx_vector_append appends another vector to the end of the provided
// vector.
//
// The svcx_vector_from_array function constructs and initializes a vector
// with the elements of the provided array. The lenght of the array and
// the stride (size of an array type) must be provided.
//
// The svcx_vector_free function frees the memory of the vector and zeroes
// it out it's data fields.
//
// The svcx_vector_size function returns the number of elements in the vector.
//
// The foreach_v and foreach_a macros allow the user to write for-each style
// loops on vectors and arrays respectively.
//
// Example:
// ```c
// svcx_arena arena;
// svcx_arena_init(&arena, 1024 * 1024);
//
// svcx_vector v;
// svcx_vector_init(&v, sizeof(int), svcx_arena_allocator(&arena));
//
// int item = 13;
// svcx_vector_push(&v, sizeof(int), &item);
// SVCX_VECTOR_PUSH(&v, int, 42);
// SVCX_VECTOR_PUSH(&v, int, 37);
// SVCX_VECTOR_PUSH(&v, int, 12);
// SVCX_VECTOR_PUSH(&v, int, 11);
//
// int val;
// svcx_vector_pop(&v, &val); // val holds last element
//
// int at_2 = *(int*)svcx_vector_at(&v, 2);
// int tmp = 69;
// svcx_vector_insert(&v, 2, &tmp);
//
// int arr[] = {1, 2, 3};
// svcx_vector v2;
// svcx_allocator def = svcx_default_allocator();
// svcx_vector_from_array(&v2, arr, SVCX_ARRAY_LEN(arr), sizeof(int), def);
// svcx_vector_append(&v, &v2);
//
// foreach_v(int *it, v2) {
//    printf("%d ", *it);
// }
//
// svcx_vector_free(&v2);
// ```
//
SVCXDEF void svcx_vector_init(svcx_vector *v, size_t stride, svcx_allocator a);
SVCXDEF void svcx_vector_clear(svcx_vector *v);
SVCXDEF svcx_result svcx_vector_reserve(svcx_vector *v, size_t min_cap);
SVCXDEF svcx_result svcx_vector_push(svcx_vector *v, const void *elem);
SVCXDEF svcx_result svcx_vector_pop(svcx_vector *v, void *out_elem);
SVCXDEF void *svcx_vector_at(svcx_vector *v, size_t index);
SVCXDEF svcx_result svcx_vector_insert(svcx_vector *v, size_t index, const void *elem);
SVCXDEF svcx_result svcx_vector_append(svcx_vector *dst, const svcx_vector *src);
SVCXDEF svcx_result svcx_vector_from_array(
    svcx_vector *v,
    const void *array,
    size_t count,
    size_t stride,
    svcx_allocator a
);
SVCXDEF void svcx_vector_free(svcx_vector *v);
SVCXDEF size_t svcx_vector_size(svcx_vector *v);

#define SVCX_VECTOR_PUSH(v, T, value)					\
    do {								\
        T tmp = (value);						\
        svcx_vector_push((v), &tmp);					\
    } while (0)

// Taken from: https://stackoverflow.com/a/400970
#define foreach_v(iter, v)						\
    for (size_t _i = 0, _keep = 1; _keep && _i < svcx_vector_size(&v);	\
         _keep = !_keep, _i++)						\
        for (iter = svcx_vector_at(&v, _i); _keep; _keep = !_keep)

#define foreach_a(iter, a)						\
    for (size_t _i = 0, _keep = 1; _keep && _i < SVCX_ARRAY_LEN(a);	\
         _keep = !_keep, _i++)						\
        for (iter = (a) + _i; _keep; _keep = !_keep)




/*
 * A string view is a non-owning and immutable data structure for representing
 * strings of characters. The value of the string view can refer to string
 * literals, string builder buffers, or substrings and is binary-safe (can
 * contain the '\0' character).
 *
 * The string view works in tandem with the string builder, which is described
 * below.
 */
typedef struct svcx_string_view {
    const char *data;
    size_t len;
} svcx_string_view;

//
// Functions for working with a string view.
//
// The svcx_sv_from_parts function allows for manual construction of a string
// view, in case this is needed. More often, users will use the next function.
//
// The svcx_sv_from_cstr function constructs a string view from the provided
// C string, automatically calculating its length.
//
// The svcx_sv_contains function is used for checking if the needle string view
// is contained in the haystack string view. If it is, the function returns
// true, otherwise it returns false.
//
// The svcx_sv_find function is used for getting the index of the first
// character of needle in the haystack. If the function could not find
// the needle in the haystack, -1 is returned.
//
// The svcx_sv_starts_with and svcx_sv_ends_with functions check if the
// string view starts or ends with the given prefix/suffix - if yes, the
// functions return true, and false otherwise.
//
// The svcx_sv_trim_start and svcx_trim_end functions trim the whitespace from
// string view from the beginning/end respectively, returning a new string view.
//
// The svcx_sv_split function splits the string view based on the provided
// delimiter and returns the different string views in the out vector. The
// out vector must be initialized before calling this function.
//
// Along with the functions, a SVCX_SV macro is provided for easier creation
// of string views.
//
// Example:
// ```c
// svcx_arena arena;
// svcx_arena_init(&arena, 1024 * 1024);
// svcx_allocator alloc = svcx_arena_allocator(&arena);
// svcx_string_view sv1 = svcx_sv_from_cstr("hello world");
// svcx_string_view sv2 = svcx_sv_from_cstr("hello");
// svcx_string_view sv3 = svcx_sv_from_cstr("world");
//
// assert(svcx_sv_starts_with(sv1, sv2));
// assert(svcx_sv_ends_with(sv1, sv3));
// assert(!svcx_sv_starts_with(sv1, sv3));
// assert(!svcx_sv_ends_with(sv1, sv2));
//
// svcx_string_view sub = svcx_sv_from_parts(sv1.data + 6, 5);
// assert(svcx_sv_ends_with(sub, svcx_sv_from_cstr("world")));
//
// svcx_string_builder sb;
// svcx_sb_init(&sb, alloc);
//
// SVCX_SB_APPEND_LIT(&sb, "Hello");
// SVCX_SB_APPEND_LIT(&sb, ", ");
// SVCX_SB_APPEND_LIT(&sb, "World");
//
// svcx_string_view exclaim = svcx_sv_from_cstr("!");
// svcx_sb_append_sv(&sb, exclaim);
//
// svcx_string_view built = svcx_sb_view(&sb);
// assert(built.len == sizeof("Hello, World!") - 1);
//
// svcx_string_view hello = svcx_sv_from_cstr("Hello");
// svcx_string_view world = svcx_sv_from_cstr("World");
//
// assert(svcx_sv_contains(built, hello));
// assert(svcx_sv_contains(built, world));
// 
// svcx_arena_free_all(&arena);
// ```
//
SVCXDEF svcx_string_view svcx_sv_from_parts(const char *data, size_t size);
SVCXDEF svcx_string_view svcx_sv_from_cstr(const char *data);
SVCXDEF bool svcx_sv_contains(
    svcx_string_view haystack,
    svcx_string_view needle
);
SVCXDEF size_t svcx_sv_find(
    svcx_string_view haystack,
    svcx_string_view needle
);
SVCXDEF bool svcx_sv_starts_with(svcx_string_view sv, svcx_string_view prefix);
SVCXDEF bool svcx_sv_ends_with(svcx_string_view sv, svcx_string_view suffix);
SVCXDEF svcx_string_view svcx_sv_trim_start(svcx_string_view sv);
SVCXDEF svcx_string_view svcx_sv_trim_end(svcx_string_view sv);
SVCXDEF svcx_string_view svcx_sv_trim(svcx_string_view sv);
SVCXDEF svcx_result svcx_sv_split(
    svcx_string_view sv,
    char delimiter,
    svcx_vector *out
);
SVCXDEF svcx_string_view svcx_sv_substring(
    svcx_string_view sv,
    size_t start,
    size_t end
);


#define SVCX_SV(lit) ((svcx_string_view){ (lit), sizeof(lit) - 1 })




/*
 * A string builder is an owning and mutable data structure for dynamically
 * creating strings of characters, but it does not provide Unicode semantics.
 * The string builder is built on top of the svcx_vector and is thus arena friendly.
 */
typedef struct svcx_string_builder {
    svcx_vector buf;
} svcx_string_builder;

//
// Functions for manipulating a string builder.
//
// The svcx_sb_init function initializes the string builder with the given
// allocator.
//
// The svcx_sb_clear function clears the data inside the string builder by
// clearing the buffer vector. This allows the string builder to be reused
// as the memory remains allocated.
//
// The svcx_sb_free function frees the memory held by the string builder,
// rendering it unusable in the future. The free function depends on the
// provided allocator, making it a no-op for arenas.
//
// The svcx_sb_push_char function pushes the given character to the end
// of the string builder.
//
// The svcx_sb_append function appends the given C string to the string
// builder. This is a manual function, meaning the size must be provided
// by the user, thus the next function will be more commonly used.
//
// The svcx_sb_append_cstr function appends the given C string to the
// string builder, automatically calculating the length. It is a wrapper
// around the svcx_sb_append function.
//
// The svcx_sb_append_sv function appends the string represented by the
// given string view to the string builder.
//
// The svcx_sb_append_fmt function is a variadic function, meaning it
// allows for an arbitrary number of parameters, and essentially acts
// as C's printf() function, allowing the user to specify a format
// string and parameters from which to construct the string to be
// appended to the string builder.
//
// The svcx_sb_cstr function returns a null-terminated view (C string)
// from the string builder's buffer. The returned string is valid until
// the next append or clear, and arena backed strings live as long as
// the arena.
//
// The svcx_sb_build function finalizes the string builder, essentially
// transfering ownership of the data to the returned pointer. The function
// returns the string constructed by the string builder. After calling this
// function, the string builder should not be used again.
//
// The svcx_sb_view function returns a non-owning string view of the data
// contained within the string builder.
//
// Alongside these functions, a SVCX_SB_APPEND_LIT macro is provided, which
// increases ergonomics of appending to the string builder.
//
// Example: An example of string builder functions is provided in the
// string view example, as they work in tandem.
//
//
SVCXDEF void svcx_sb_init(svcx_string_builder *sb, svcx_allocator a);
SVCXDEF void svcx_sb_clear(svcx_string_builder *sb);
SVCXDEF void svcx_sb_free(svcx_string_builder *sb);
SVCXDEF svcx_result svcx_sb_push_char(svcx_string_builder *sb, char c);
SVCXDEF svcx_result svcx_sb_append(
    svcx_string_builder *sb,                                   
    const char *str,
    size_t len
);
SVCXDEF svcx_result svcx_sb_append_cstr(
    svcx_string_builder *sb,
    const char *cstr
);
SVCXDEF svcx_result svcx_sb_append_sv(
    svcx_string_builder *sb,
    svcx_string_view sv
);
SVCXDEF svcx_result svcx_sb_append_fmt(
    svcx_string_builder *sb,
    const char *fmt,
    ...
);
SVCXDEF const char *svcx_sb_cstr(svcx_string_builder *sb);
SVCXDEF char *svcx_sb_build(svcx_string_builder *sb);
SVCXDEF svcx_string_view svcx_sb_view(svcx_string_builder *sb);

#define SVCX_SB_APPEND_LIT(sb, lit) \
    svcx_sb_append_sv((sb), SVCX_SV(lit))




#ifdef SVCX_IMPLEMENTATION




SVCXDEF const char *svcx_error_string(svcx_result r) {
    switch (r) {
    case SVCX_OK:
      return "no error";
    case SVCX_VEC_GROW_MEM_ERR:
      return "vector grow could not allocate memory";
    case SVCX_VEC_ERR_OOM:
	return "vector reserve out of memory";
    case SVCX_VEC_PUSH_GROW_ERR:
      return "vector push could not grow vector";
    case SVCX_VEC_POP_EMPTY_ERR:
      return "vector is empty on pop";
    case SVCX_VEC_INSERT_OOB:
      return "index out of bounds on vector insert";
    case SVCX_VEC_INSERT_GROW_ERR:
      return "vector insert could not grow memory";
    case SVCX_VEC_APPEND_STRIDE_ERR:
      return "vectors differ in stride values on append";
    case SVCX_VEC_APPEND_GROW_ERR:
      return "vector append could not grow vector";
    case SVCX_VEC_FROM_ARR_MALLOC_ERR:
      return "vector from array could not malloc memory";
    case SVCX_SV_SPLIT_ERR:
      return "string view split could not push to output vector";
    case SVCX_SB_PUSHC_ERR:
      return "string builder could not push char to internal vector";
    case SVCX_SB_APPEND_ERR:
      return "string builder could not reserve memory for append";
    case SVCX_SB_APPEND_SV_ERR:
      return "string builder could not reserve memory for string view append";
    case SVCX_SB_FMT_INVALID_ARG_ERR:
      return "string builder invalid arguments provided to format";
    case SVCX_SB_FMT_RESERVE_ERR:
      return "string builder could not reserve memory for format";
    default:
      return "unknown error";      
    }  
}




SVCXDEF void *svcx__malloc(void *ctx, size_t size) {
    SVCX_UNUSED(ctx);
    return malloc(size);
}

SVCXDEF void *svcx__realloc(void *ctx, void *ptr, size_t size) {
    SVCX_UNUSED(ctx);
    return realloc(ptr, size);
}

SVCXDEF void svcx__free(void *ctx, void *ptr) {
    SVCX_UNUSED(ctx);
    free(ptr);
}


SVCXDEF svcx_allocator svcx_default_allocator(void) {
    svcx_allocator a = {
        .alloc = svcx__malloc,
        .realloc = svcx__realloc,
        .free = svcx__free,
        .ctx = NULL,
    };
    return a;
}

SVCXDEF void *svcx_alloc(svcx_allocator *a, size_t size) {
    return a->alloc(a->ctx, size);
}

SVCXDEF void *svcx_realloc(svcx_allocator *a, void *ptr, size_t size) {
    return a->realloc(a->ctx, ptr, size);
}

SVCXDEF void svcx_free(svcx_allocator *a, void *ptr) {
    a->free(a->ctx, ptr);
}

SVCXDEF bool svcx_allocator_is_valid(const svcx_allocator *a) {
    return a && a->alloc && a->free;
}

SVCXDEF void *svcx_alloc_zero(svcx_allocator *a, size_t size) {
    void *p = svcx_alloc(a, size);
    if (p) {
        memset(p, 0, size);      
    }   
    return p;
}




SVCXDEF svcx_allocator svcx_arena_allocator(svcx_arena *arena) {
    svcx_allocator a = {
        .alloc = svcx_arena_alloc,
        .realloc = svcx_arena_realloc,
        .free = svcx_arena_free,
        .ctx = arena
    };
    return a;
}

SVCXDEF void svcx_arena_init(svcx_arena *arena, size_t size) {
    arena->base = malloc(size);
    arena->size = size;
    arena->used = 0;
}

SVCXDEF void *svcx_arena_alloc(void *ctx, size_t size) {
    svcx_arena *arena = (svcx_arena *)ctx;

    size = (size + 7) & ~7; // Align to 8 bytes

    if (arena->used + size > arena->size) {
        return NULL;
    }

    void *ptr = arena->base + arena->used;
    arena->used += size;
    return ptr;
}

SVCXDEF void *svcx_arena_realloc(void *ctx, void *ptr, size_t size) {
    SVCX_UNSUPPORTED("Reallocation is not supported in arenas.");
    SVCX_UNUSED(ctx);
    SVCX_UNUSED(ptr);
    SVCX_UNUSED(size);
    return NULL;
}

SVCXDEF void svcx_arena_free(void *ctx, void *ptr) {
    SVCX_UNUSED(ctx);
    SVCX_UNUSED(ptr);
    // A free in an arena is a no-op
}

SVCXDEF void svcx_arena_reset(svcx_arena *arena) { arena->used = 0; }

SVCXDEF void svcx_arena_free_all(svcx_arena *arena) {
    free(arena->base);
    arena->size = 0;
    arena->used = 0;
}




SVCXDEF svcx_result _svcx_vector_grow(svcx_vector *v, size_t min_cap) {
    size_t new_cap = v->cap ? v->cap * 2 : 8;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }

    // We cannot use svcx_realloc here in case the user uses an allocator
    // that does not support realloc (like arena).
    // void *new_data = svcx_realloc(&v->a, v->data, new_cap * v->stride);

    size_t new_size = new_cap * v->stride;
    void *new_data;
    if (v->data) {
        new_data = svcx_alloc(&v->a, new_size);
        if (!new_data) {
            return SVCX_VEC_GROW_MEM_ERR;
        }

        memcpy(new_data, v->data, v->size * v->stride);
    } else {
        new_data = svcx_alloc(&v->a, new_size);
        if (!new_data) {
            return SVCX_VEC_GROW_MEM_ERR;
        }
    }
    
    v->data = new_data;
    v->cap = new_cap;
    return SVCX_OK;
}

SVCXDEF void svcx_vector_init(svcx_vector *v, size_t stride, svcx_allocator a) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
    v->stride = stride;
    v->a = a;
}

SVCXDEF void svcx_vector_clear(svcx_vector *v) {
    SVCX_ASSERT(v);
    v->size = 0;
}

SVCXDEF svcx_result svcx_vector_reserve(svcx_vector *v, size_t min_cap) {
    if (min_cap <= v->cap) {
	return SVCX_OK;
    }

    size_t new_cap = v->cap ? v->cap : 8;

    while (new_cap < min_cap) {
	new_cap *= 2;
    }

    size_t new_size_bytes = new_cap * v->stride;

    void *new_data = svcx_alloc(&v->a, new_size_bytes);
    if (!new_data) {
	return SVCX_VEC_ERR_OOM;
    }

    if (v->data && v->size > 0) {
	memcpy(new_data, v->data, v->size * v->stride);
    }

    if (v->data) {
	svcx_free(&v->a, v->data);
    }

    v->data = new_data;
    v->cap = new_cap;
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_vector_push(svcx_vector *v, const void *elem) {
    SVCX_ASSERT(v);
    
    if (v->size == v->cap) {
        int err = _svcx_vector_grow(v, v->size + 1);
        if (err != 0) {
            return SVCX_VEC_PUSH_GROW_ERR;
        }        
    }

    void *dst = (char *)v->data + v->size * v->stride;
    memcpy(dst, elem, v->stride);
    v->size++;
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_vector_pop(svcx_vector *v, void *out_elem) {
    SVCX_ASSERT(v);
    
    if (v->size == 0) {
        return SVCX_VEC_POP_EMPTY_ERR;
    }

    v->size--;

    if (out_elem) {
        void *src = (char *)v->data + v->size * v->stride;
        memcpy(out_elem, src, v->stride);
    }

    return SVCX_OK;
}

SVCXDEF void *svcx_vector_at(svcx_vector *v, size_t index) {
    SVCX_ASSERT(v);

    if (index >= v->size) {
        return NULL;
    }

    return (char *)v->data + index * v->stride;
}

SVCXDEF svcx_result svcx_vector_insert(svcx_vector *v, size_t index, const void *elem) {
    SVCX_ASSERT(v);
    if (index > v->size) {
        return SVCX_VEC_INSERT_OOB;
    }

    if (v->size == v->cap) {
        int err = _svcx_vector_grow(v, v->size + 1);
        if (err != 0) {
            return SVCX_VEC_INSERT_GROW_ERR;
        }
    }

    void *dst = (char *)v->data + index * v->stride;
    memmove((char *)dst + v->stride, dst, (v->size - index) * v->stride);

    memcpy(dst, elem, v->stride);
    v->size++;
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_vector_append(svcx_vector *dst, const svcx_vector *src) {
    SVCX_ASSERT(dst);
    SVCX_ASSERT(src);

    if (dst->stride != src->stride) {
        return SVCX_VEC_APPEND_STRIDE_ERR;
    }

    size_t new_size = dst->size + src->size;

    if (new_size > dst->cap) {
        int err = _svcx_vector_grow(dst, new_size);
        if (err != 0) {
            return SVCX_VEC_APPEND_GROW_ERR;
        }
    }

    memcpy(
        (char *)dst->data + dst->size * dst->stride,
        src->data,
        src->size * src->stride                           
    );
    dst->size = new_size;
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_vector_from_array(
    svcx_vector *v,
    const void *array,
    size_t count,
    size_t stride,
    svcx_allocator a
) {
    svcx_vector_init(v, stride, a);

    if (count == 0) {
        return 0;
    }

    v->data = (void *)svcx_alloc(&v->a, count * stride);
    if (!v->data) {
        return SVCX_VEC_FROM_ARR_MALLOC_ERR;
    }

    memcpy(v->data, array, count * stride);
    v->size = count;
    v->cap = count;
    return SVCX_OK;
}

SVCXDEF void svcx_vector_free(svcx_vector *v) {
    SVCX_ASSERT(v);
    
    if (v->data) {
        svcx_free(&v->a, v->data);
    }

    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

SVCXDEF size_t svcx_vector_size(svcx_vector *v) {
    SVCX_ASSERT(v);
    return v->size;
}




SVCXDEF svcx_string_view svcx_sv_from_parts(const char *data, size_t size) {
    svcx_string_view sv;
    sv.data = data;
    sv.len = size;
    return sv;
}

SVCXDEF svcx_string_view svcx_sv_from_cstr(const char *data) {
    return (svcx_string_view) { data, strlen(data) };
}

SVCXDEF bool svcx_sv_contains(
    svcx_string_view haystack,
    svcx_string_view needle
) {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
	return false;
    }

    for (size_t i = 0; i <= haystack.len - needle.len; i++) {
        if (memcmp(haystack.data + i, needle.data, needle.len) == 0) {
	    return true;
	}
    }
    return false;
}

SVCXDEF size_t svcx_sv_find(
    svcx_string_view haystack,
    svcx_string_view needle
) {
    if (needle.len == 0) {
	return 0;
    }
    if (needle.len > haystack.len) {
	return -1;
    }

    for (size_t i = 0; i <= haystack.len - needle.len; i++) {
	if (memcmp(haystack.data + i, needle.data, needle.len) == 0) {
	    return i;
        }
    }
    return -1;
}

SVCXDEF bool svcx_sv_starts_with(svcx_string_view sv, svcx_string_view prefix) {
    if (prefix.len > sv.len) {
	return false;
    }
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

SVCXDEF bool svcx_sv_ends_with(svcx_string_view sv, svcx_string_view suffix) {
    if (suffix.len > sv.len) {
	return false;
    }
    
    const char *start = sv.data + (sv.len - suffix.len);
    return memcmp(start, suffix.data, suffix.len) == 0;
}

SVCXDEF svcx_string_view svcx_sv_trim_start(svcx_string_view sv) {
    size_t start = 0;
    size_t end = sv.len;

    while (start < end && isspace((unsigned char) sv.data[start])) {
	start++;
    }
    
    return (svcx_string_view) {
        .data = sv.data + start,
	.len = end - start
    };
}

SVCXDEF svcx_string_view svcx_sv_trim_end(svcx_string_view sv) {
    size_t start = 0;
    size_t end = sv.len;

    while (end > start && isspace((unsigned char)sv.data[end - 1])) {
	end--;
    }

    return (svcx_string_view) {
        .data = sv.data,
	.len = end,
    };
}

SVCXDEF svcx_string_view svcx_sv_trim(svcx_string_view sv) {
    svcx_string_view tmp = svcx_sv_trim_start(sv);
    return svcx_sv_trim_end(tmp);
}

SVCXDEF svcx_result svcx_sv_split(
    svcx_string_view sv,
    char delimiter,
    svcx_vector *out
) {
    size_t start = 0;

    for (size_t i = 0; i <= sv.len; i++) {
        if (i == sv.len || sv.data[i] == delimiter) {
            svcx_string_view part = {
		.data = sv.data + start,
		.len = i - start,
	    };

            svcx_result r = svcx_vector_push(out, &part);
	    if (r != SVCX_OK) {
		return SVCX_SV_SPLIT_ERR;
            }

	    start = i + 1;
	}
    }

    return SVCX_OK;
}

SVCXDEF svcx_string_view svcx_sv_substring(
    svcx_string_view sv,
    size_t start,
    size_t end
) {
    SVCX_ASSERT(start <= end);
    SVCX_ASSERT(end <= sv.len);

    if (start > end) {
	start = end;
    }
    if (end > sv.len) {
	end = sv.len;
    }

    return svcx_sv_from_parts(sv.data + start, end - start);
}




SVCXDEF void svcx_sb_init(svcx_string_builder *sb, svcx_allocator a) {
    svcx_vector_init(&sb->buf, sizeof(char), a);
}

SVCXDEF void svcx_sb_clear(svcx_string_builder *sb) {
    svcx_vector_clear(&sb->buf);
}

SVCXDEF void svcx_sb_free(svcx_string_builder *sb) {
    svcx_vector_free(&sb->buf);
}

SVCXDEF svcx_result svcx_sb_push_char(svcx_string_builder *sb, char c) {
    if (svcx_vector_push(&sb->buf, &c) != SVCX_OK) {
	return SVCX_SB_PUSHC_ERR;
    }
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_sb_append(
    svcx_string_builder *sb,                                   
    const char *str,
    size_t len
) {
    SVCX_ASSERT(str || len == 0);

    if (len == 0) {
	return SVCX_OK;
    }

    size_t old_size = sb->buf.size;

    svcx_result r = svcx_vector_reserve(&sb->buf, old_size + len);

    if (r != SVCX_OK) {
	return SVCX_SB_APPEND_ERR;
    }

    memcpy((char *)sb->buf.data + old_size, str, len);
    sb->buf.size += len;

    return SVCX_OK;
}

SVCXDEF svcx_result svcx_sb_append_cstr(
    svcx_string_builder *sb,
    const char *cstr
) {
    return svcx_sb_append(sb, cstr, strlen(cstr));
}

SVCXDEF svcx_result svcx_sb_append_sv(
    svcx_string_builder *sb,
    svcx_string_view sv
) {
    if (sv.len == 0) {
	return SVCX_OK;
    }

    size_t new_size = sb->buf.size + sv.len;

    svcx_result r = svcx_vector_reserve(&sb->buf, new_size);
    if (r != SVCX_OK) {
	return SVCX_SB_APPEND_SV_ERR;
    }

    memcpy(
	   (char *)sb->buf.data + sb->buf.size,
           sv.data,
	   sv.len
    );

    sb->buf.size = new_size;
    
    return SVCX_OK;
}

SVCXDEF svcx_result svcx_sb_append_fmt(
    svcx_string_builder *sb,
    const char *fmt,
    ...
) {
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed < 0) {
        va_end(args_copy);
	return SVCX_SB_FMT_INVALID_ARG_ERR;
    }

    size_t old_size = sb->buf.size;

    svcx_result r = svcx_vector_reserve(&sb->buf, old_size + (size_t)needed);

    if (r != SVCX_OK) {
        va_end(args_copy);
	return SVCX_SB_FMT_RESERVE_ERR;
    }

    vsnprintf(
        (char *)sb->buf.data + old_size,
	(size_t)needed + 1,
	fmt,
	args_copy
    );
    va_end(args_copy);
    sb->buf.size += (size_t)needed;
    return SVCX_OK;
}

SVCXDEF const char *svcx_sb_cstr(svcx_string_builder *sb) {
    char zero = '\0';

    if (sb->buf.size == sb->buf.cap ||
        ((char *)sb->buf.data)[sb->buf.size] != '\0') {
        svcx_vector_push(&sb->buf, &zero);
	sb->buf.size--;
    }

    return (const char *)sb->buf.data;
}

SVCXDEF char *svcx_sb_build(svcx_string_builder *sb) {
    char zero = '\0';
    svcx_vector_push(&sb->buf, &zero);
    return (char *)sb->buf.data;
}

SVCXDEF svcx_string_view svcx_sb_view(svcx_string_builder *sb) {
    return (svcx_string_view) {
        .data = (const char *)sb->buf.data,
	.len = sb->buf.size
    };
}


    


#endif // SVCX_IMPLEMENTATION

#endif // SVCXTEND_H
