/*
* SPDX-License-Identifier: GPL-2.0
*/

#ifndef R_CLIBS_H
#define R_CLIBS_H

#include <stdio.h>
#include <float.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_MSC_VER) && !defined(HL_LLVM)
#if !defined(_WIN64)
#	pragma warning(disable:4996) // remove deprecated C API usage warnings
#endif
#endif

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
#   ifndef snprintf
#       define snprintf _snprintf
#   endif
#   define ALIGNED_(x) __declspec(align(x))
#   define VLADecl(type, name, len) type *name = _alloca(sizeof(type) * (len))
#else
#   define ALIGNED_(x) __attribute__ ((aligned(x)))
#   define VLADecl(type, name, len) type name[len]
#endif

#ifndef ARRAYSIZE
#   define ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif
#define NOT_ALIGNED(size, pow2)   (size & (pow2 - 1))
#define ALIGN_POW2(size, pow2)    ((((size) - 1) | (pow2 - 1)) + 1)

// Below code works fine for most current environments
#if defined(__LP64__) || defined(_WIN64) || defined(_M_X64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__) || defined(__ppc64__)
#   define IS64BIT 1
#else
#   define IS32BIT 1
#endif

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