# svcxtend

SVCXtend - Sveigl's C eXtended is a small, header-only personal utility library for C development inspired by the stb libraries.

## Contents

The library contains various utilities that make development in C more ergonomic:
- Allocators (default and arena)
- Vector (essentially a dynamic array of any type)
- String builder (owning) and string view (non-owning)
- Various utility macros
- More stuff, like a hashmap, will be added

## Usage

To use this library, simply include the header file in your C file and compile the project. Note, however, that this only includes the declarations of functions, but not their implementations. To include the implementations, include the library like so:

```c
#define SVCX_IMPLEMENTATION
#include "svcxtend.h"
```

Additionally, users can define the SVCX_DEBUG macro to enable assertions in SVCX function calls, thereby verifying the arguments passed into them in debug versions. If the SVCX_DEBUG macro is not defined, the assertions become no-ops.

All functions in the library are prefixed with SVCXDEF. This means, that the user can define the SVCXDEF macro to prepend the functions with different keywords, for example:

```c
#define SVCXDEF static inline
```
