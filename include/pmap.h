/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 * Copyright (C) 2025 LWM
 *
 * To use pmap you'll have to implement your own insert, remove, search and iterater routines.
 * This will avoid us to use callbacks and to drop drammatically performances.
 * I know it's not the cleaner way, but in C (not in C++) to get performances and genericity...
 *
 * Refer to `test/pmap_test.c` for samples.
 */

#ifndef LWM_PMAP_H
#define LWM_PMAP_H
#include <stddef.h>

/*
 * The capacity is about "2 ^ (height * 0.75)"(inaccurate)
 */
#ifndef PMAP_STACK_HEIGHT
#   define PMAP_STACK_HEIGHT 32
#endif

struct pmnode {
	struct pmnode *left;
	struct pmnode *right;
	int height;
	int aux; // unused align field, available for storing arbitrary values.
};

 int pmap_count(struct pmnode *root);
void pmap_balance(struct pmnode ***stacks, int index);
void pmap_remove(struct pmnode **slot, struct pmnode ***stacks, int index);

static int inline pmap_height(struct pmnode *node)
{
	return node ? node->height : 0;
}

#ifndef container_of
#   if defined(_MSC_VER) || !defined(__llvm__) // unsafe in msvc
#       define container_of(ptr, type, member) \
            ((type *)((char *)ptr - offsetof(type, member)))
#   else
#       define container_of(ptr, type, member) ({ \
            const __typeof__(((type *)0)->member) * __mptr = (ptr); \
            (type *)((char *)ptr - offsetof(type, member)); })
#   endif
#endif

#endif
