#ifndef stack_1LKrZ2oprBaT
#define stack_1LKrZ2oprBaT



#include <err.h>
#include <stdlib.h>



/* This is not assert: It is not intended to be disabled in non-debug
   mode.  Use this for acceptable runtime errors to terminate the
   program. */

#define ERRIF(c) do {                           \
        if (c)                                  \
            err(1, "Fatal " #c " at " __FILE__  \
                ":%d; errno", __LINE__);        \
    } while (0)


/* An array-based stack.  `STACK(foo)` is the type, so `STACK(int)
   bar` declares variable `bar` to be a stack of integers.  The stack
   is not initialised. */

#define STACK(ty) struct { size_t alloc, used; ty *array; }

/* Allocate memory in stack `foo` for `s` entries.  The stack is
   empty.  Terminates the program if `malloc` fails. */

#define ALLOCATE(foo, s) do {                                   \
        foo.alloc = s;                                          \
        foo.used = 0;                                           \
        foo.array = malloc(foo.alloc * sizeof(*foo.array));     \
        ERRIF(!foo.array);                                      \
    } while (0)


/* If stack `foo` is full, double its size.  Terminates the program if
   `realloc` fails. */

#define ENOUGH(foo) do {                                                \
        if (foo.used >= foo.alloc) {                                    \
            foo.alloc *= 2;                                             \
            foo.array = realloc(foo.array, foo.alloc * sizeof(*foo.array)); \
            ERRIF(!foo.array);                                          \
        }                                                               \
    } while (0)


/* Reduce allocated memory of stack to the minimum required to contain
   its items.  If stack `foo` is emty, free memory.  Terminates
   the program if `realloc` fails. */

#define TRIM(foo) do {                                                  \
        foo.alloc = foo.used;                                           \
        if (foo.alloc) {                                                \
            foo.array = realloc(foo.array, foo.alloc * sizeof(*foo.array)); \
            ERRIF(!foo.array);                                          \
        } else {                                                        \
            free(foo.array);                                            \
            foo.array = NULL;                                           \
        }                                                               \
    } while (0)


/* Push `val` on stack `foo`.  The result is undefined if the stack is
   not large enough.  No check is performed! */

#define PUSH(foo, val) (foo.array[foo.used++] = (val))


/* Get number of entries on the stack */

#define COUNT(foo) (foo.used)


/* Pop value from stack `foo`.  The result is undefined if the stack
   is empty.  No check is performed! */

#define POP(foo) (foo.array[--foo.used])


/* Get `idx`-th value from stack.  The result is undefined if the
   stack is not large enough.  No check is performed! */

#define AT(foo, idx) (foo.array[idx])



#endif
