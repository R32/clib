#ifndef COMM_H
#define COMM_H

#include <locale.h>
#include <wchar.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef C_FUNCTION_BEGIN
#   ifdef __cplusplus
#       define C_FUNCTION_BEGIN extern "C" {
#       define C_FUNCTION_END };
#   else
#       define C_FUNCTION_BEGIN
#       define C_FUNCTION_END
#   endif
#endif

#ifndef container_of
#   if defined(_MSC_VER) || !defined(__llvm__) // unsafe in msvc
#       define container_of(ptr, type, member)\
        ((type *)((char *)ptr - offsetof(type, member)))
#   else
#   define container_of(ptr, type, member) ({\
        const __typeof__(((type *)0)->member) * __mptr = (ptr);\
        (type *)((char *)ptr - offsetof(type, member)); })
#   endif
#endif

#if !defined(inline) && defined(_MSC_VER)
#   define inline __inline
#endif

#ifndef __always_inline
#   ifdef _MSC_VER
#       define __always_inline __forceinline
#   else
#       define __always_inline __attribute__((always_inline))
#   endif
#endif

#ifdef _MSC_VER
#   define snprintf _snprintf
#   define ALIGNED_(x) __declspec(align(x))
#else
#   define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#if ((ULONG_MAX) == (0xFFFFFFFFUL))
#   define IS_32
#else
#   define IS_64
#endif

// singly linked list
struct slist_head {
	struct slist_head* next;
};

// doubly linked list
struct list_head {
	struct list_head *next, *prev;
};

// used for hashtable
struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

// some compatibility for files copied from linux
#ifndef READ_ONCE
#   define READ_ONCE(x) (x)
#   define WRITE_ONCE(x, val) x=(val)
#   define LIST_POISON1 NULL
#   define LIST_POISON2 NULL
#endif

#ifndef unlikely
#   define likely(x) (x)
#   define unlikely(x) (x)
#endif

#endif