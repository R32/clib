/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_TINYALLOC_H
#define R_TINYALLOC_H
#include "rclibs.h"

#ifndef TINYALLOC_BLK_BASE
#	define TINYALLOC_BLK_BASE          8
#endif
#ifndef TINYALLOC_FREELIST_MAX
#	define TINYALLOC_FREELIST_MAX      16
#endif

#ifndef rt_malloc
#	define rt_malloc malloc
#endif
#ifndef rt_free
#	define rt_free free
#endif

// private
struct allocator_base {
	int chksize; // in KB
	int metasize;
	void *chunk_head;
};

struct tinyalloc_root {
	struct allocator_base base;
	void *freelist[TINYALLOC_FREELIST_MAX];
};

struct bumpalloc_root {
	struct allocator_base base;
};

struct fixedalloc_root {
	struct allocator_base base;
	int size;
	void *freelist[1];
};

C_FUNCTION_BEGIN

/*
 * @chksize: The KB size of each chunk
 *
 * ```c
 * // struct initialization
 * struct tinyalloc_root tiny = {
 *	.chksize = 32,            // In Kb, You could adjust this value according to your needs
 *	.metasize = sizeof(int),  // Same as sizeof(struct meta)
 * // The compiler will automatically set the remaining fields to NULL(0)
 * };
 * ```
 */
void tinyalloc_init(struct tinyalloc_root *fixed, int chksize);

void tinyfree(struct tinyalloc_root *root, void *ptr);

void *tinyalloc(struct tinyalloc_root *root, int size);

void tinyreset(struct tinyalloc_root *root);

void tinydestroy(struct tinyalloc_root *root);

/*
 * bump allocator
 *
 * ```c
 * struct bumpalloc_root bump = { .chksize = 4 };
 * ```
 */
void bumpalloc_init(struct bumpalloc_root *fixed, int chksize);

void *bumpalloc(struct bumpalloc_root *bump, int size);

void bumpreset(struct bumpalloc_root *bump);

void bumpdestroy(struct bumpalloc_root *bump);

/*
 * fixed allocator
 *
 * ```c
 * struct fixedalloc_root fixed = { .chksize = 4 };
 * ```
 */
void fixedalloc_init(struct fixedalloc_root *fixed, int chksize, int size);

void *fixedalloc(struct fixedalloc_root *fixed);

void fixedfree(struct fixedalloc_root *fixed, void *ptr);

void fixedreset(struct fixedalloc_root *fixed);

void fixeddestroy(struct fixedalloc_root *fixed);

C_FUNCTION_END
#endif
