/*
 * svcxtend - Sveigl's C eXtended
 *
 * This is a small library containing various utility functions for C
 * development.
 *
 * # Small example:
 *
 * ```c
 * #define SVCX_IMPLEMENTATION
 * #include "svcxtend.h"
 *
 * int main () {
 *     svcx_allocator a = svcx_default_allocator();
 *     int *ptr = svcx_alloc(&a, 10 * sizeof(10));
 *     ...
 * }
 * ```
 * # Macro interface
 *
 * This library exposes multiple macros that can be defined by the user
 * to change various aspects of the functionality. The list of macros:
 *
 * - SVCX_IMPLEMENTATION - Enables the definition of functions, if not
 *   defined as in the example, only function declarations are included.
 * - SVCXDEF - Can be redefined to append additional flags to function
 *   declarations, for example `#define SVCXDEF static inline` will
 *   append the static inline keywords to all functions in the file.
 *
 *
 * # Contents
 *
 * This library contains various utilities for C development, such as:
 * - A default allocator
 * - An arena allocator
 * - A vector
 * - Rest to be added
 */

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

/*
 * Some helper macros to make code more ergonomic.
 *
 * The SVCX_UNUSED macro can be used in functions to suppress unused argument
 * warnings:
 * ```c
 * int foo(int unused) {
 *     SVCX_UNUSED(unused);
 *     return 0;
 * }
 * ```
 *
 * The SVCX_UNSUPPORTED macro can be used within functions that have to exist to
 * satisfy the compiler but should not be used.
 *
 * The SVCX_ARRAY_LEN macro is here to make getting array length less cluttered.
 */
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
    SVCX_VEC_PUSH_GROW_ERR,
    SVCX_VEC_POP_EMPTY_ERR,
    SVCX_VEC_INSERT_OOB,
    SVCX_VEC_INSERT_GROW_ERR,
    SVCX_VEC_APPEND_STRIDE_ERR,
    SVCX_VEC_APPEND_GROW_ERR,
    SVCX_VEC_FROM_ARR_MALLOC_ERR
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

/*
 * Internal default allocator functions. These can be safely ignored.
 * To interact with the allocators, use the functions in the block
 * beneath this one.
 */
SVCXDEF void *svcx__malloc(void *ctx, size_t size);
SVCXDEF void *svcx__realloc(void *ctx, void *ptr, size_t size);
SVCXDEF void svcx__free(void *ctx, void *ptr);


/*
 * The allocator helper functions, to be used in actual code.
 *
 * The svcx_default_allocator returns the default allocator, which
 * supports all three key operations: allocation, reallocation and
 * freeing.
 *
 * The svcx_alloc function performs the allocator's allocation operation
 * and returns a pointer to the allocated memory.
 *
 * The svcx_realloc function performs the reallocation operation
 * of the allocator, if one is available (for example, the arena
 * allocator does not support reallocation.
 *
 * The svcx_free function performs the free operation on the provided
 * pointer, if the allocator supports it (for example, in the arena
 * allocator, this operation is a no-op and does nothing).
 *
 * The svcx_allocator_is_valid function checks if the custom defined
 * allocator is valid.
 *
 * The svcx_alloc_zero function uses the allocation function of the
 * provided allocator, while also setting the allocated memory to
 * zero.
 *
 * Example:
 * ```c
 * svcx_allocator a = svcx_default_allocator();
 * int *ptr = (int *)svcx_alloc(&a, 10 * sizeof(int));
 * ptr = (int *)svcx_realloc(&a, 20 * sizeof(int));
 * svcx_free(&a, ptr);
 * ```
 */
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

/*
 * Internal arena allocator functions. These can be safely ignored.
 * To create the arena and free it, use the functions in the block
 * below.
 */
SVCXDEF void *svcx_arena_alloc(void *ctx, size_t size);
SVCXDEF void *svcx_arena_realloc(void *ctx, void *ptr, size_t size);
SVCXDEF void svcx_arena_free(void *ctx, void *ptr);


/*
 * The arena helper functions, to be used in actual code.
 *
 * The svcx_arena_allocator returns an arena allocator, which
 * holds the provided arena and uses it for memory allocation.
 *
 * The svcx_arena_init function performs the initialization of
 * the arena, which can be passed into the allocator.
 *
 * The svcx_arena_reset function resets the arena's used counter
 * effectively clearing the data an starting the process of
 * overwriting it.
 *
 * The svcx_arena_free_all function frees the arena's memory
 * and reseting it's fields, completely zeroing it out.
 *
 * Example:
 * ```c
 * svcx_arena arena = {0};
 * svcx_arena_init(&a, 1024 * 1024);
 * svcx_allocator a = svcx_arena_allocator(&arena);
 * int *ptr = svcx_alloc(&a, 1024);
 * if (!ptr) {
 *     printf("Arena memory depleted\n");
 * }
 * svcx_arena_reset(&arena);
 * int *ptr = svcx_alloc(&a, 1024);
 * svcx_arena_Free_all(&arena);
 * // Arena invalid here, must be reinitialized
 * ```
 */
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


/*
 * Functions for manipulating a vector. Some of these functions might be
 * slightly unergonomic to use, and ways to use them effectively are
 * shown in the example below.
 *
 * The svcx_vector_init function initializes the given vector with the provided
 * allocator. Stride needs to be provided in order to enable the vector to work
 * with any type, and is essentially just the size of the data type in the
 * vector.
 *
 * The svcx_vector_clear function resets the vector's size count to 0,
 * effectively discarding the data and starting the overwriting process.
 *
 * The svcx_vector_push function takes in a pointer to the element to be
 * added to the end of the vector. Because this can be cumbersome, a macro
 * SVCX_VECTOR_PUSH(v, T, value) is provided, allowing the user to pass in
 * actual values.
 *
 * The svcx_vector_pop function removes the last element from the vector and
 * returns it in the out_elem pointer.
 *
 * The svcx_vector_at function returns a pointer to the element at the given
 * index.
 *
 * The svcx_vector_insert function inserts the given element at the provided
 * index. Same as the push function, this function accepts a pointer to the
 * element to enable the vector to work with any type, however no wrapper
 * macro is provided for this function, as this operation is usually not
 * that common.
 *
 * The svcx_vector_append appends another vector to the end of the provided
 * vector.
 *
 * The svcx_vector_from_array function constructs and initializes a vector
 * with the elements of the provided array. The lenght of the array and
 * the stride (size of an array type) must be provided.
 *
 * The svcx_vector_free function frees the memory of the vector and zeroes
 * it out it's data fields.
 *
 * Example:
 * ```c
 * svcx_arena arena;
 * svcx_arena_init(&arena, 1024 * 1024);
 *
 * svcx_vector v;
 * svcx_vector_init(&v, sizeof(int), svcx_arena_allocator(&arena));
 *
 * int item = 13;
 * svcx_vector_push(&v, sizeof(int), &item);
 * SVCX_VECTOR_PUSH(&v, int, 42);
 * SVCX_VECTOR_PUSH(&v, int, 37);
 * SVCX_VECTOR_PUSH(&v, int, 12);
 * SVCX_VECTOR_PUSH(&v, int, 11);
 *
 * int val;
 * svcx_vector_pop(&v, &val); // Val holds last element
 *
 * int at_2 = *(int*)svcx_vector_at(&v, 2);
 * int tmp = 69;
 * svcx_vector_insert(&v, 2, &tmp);
 *
 * int arr[] = {1, 2, 3};
 * svcx_vector v2;
 * svcx_allocator def = svcx_default_allocator();
 * svcx_vector_from_array(&v2, arr, SVCX_ARRAY_LEN(arr), sizeof(int), def;
 * svcx_vector_append(&v, &v2);
 * svcx_vector_free(&v2);
 * ```
 */
SVCXDEF void svcx_vector_init(svcx_vector *v, size_t stride, svcx_allocator a);
SVCXDEF void svcx_vector_clear(svcx_vector *v);
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




#ifdef SVCX_IMPLEMENTATION




SVCXDEF const char *svcx_error_string(svcx_result r) {
    switch (r) {
    case SVCX_OK:
      return "no rerror";
    case SVCX_VEC_GROW_MEM_ERR:
      return "vector grow could not allocate memory";
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


#endif // SVCX_IMPLEMENTATION

#endif // SVCXTEND_H
