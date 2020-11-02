#ifndef COMM_H
#define COMM_H

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

#ifdef _MSC_VER
#   define inline __inline
#   define snprintf _snprintf
#   define ALIGNED_(x) __declspec(align(x))
#else
#   define ALIGNED_(x) __attribute__ ((aligned(x)))
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

#endif