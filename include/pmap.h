/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 */
#ifndef R32_PMAP_H
#define R32_PMAP_H

struct pmnode {
	struct pmnode **rope;
	struct pmnode *left;
	struct pmnode *right;
	int height;
};

struct pmnode *pmap_balance(struct pmnode **slot);
struct pmnode *pmap_merge(struct pmnode **slot);


#ifndef NULL
#   define NULL 0
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

#endif
