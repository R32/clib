/*
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdlib.h>
#include <string.h>
#include "tinyalloc.h"

#define BLK_BASE           TINYALLOC_BLK_BASE
#define FREELIST_MAX       TINYALLOC_FREELIST_MAX

struct meta {
	int size; // sizeof(meta->data)
	char data[0];
};

#define META_DATAPTR(m)    ((m)->data)
#define META_DATASIZE(m)   ((m)->size)
#define META_FULLSIZE(m)   ((m)->size + sizeof(struct meta))
#define FREE_NEXT(m)       (*(void **)(m)->data)
#define FREE_HEAD(root, i) ((root)->freelist[i])
#define FREE_RESET(fl)     (freelist_reset(fl, ARRAYSIZE(fl)))
#define N1024              1024

struct chunk {
	int pos;
	int size; // sizeof(chunk->mem)
	union {
		struct chunk *next;
		double __x; // align(struct chunk) to 8bytes if compiler is 32bit
	};
	char mem[0];
};

#define the_base(ator)     (&ator->base)
#define chk_next(chk)      ((chk)->next)
#define chk_head(base)     ((base)->chunk_head)
#define chk_dataptr(chk)   ((chk)->mem + (chk)->pos)

static inline void chunk_add(struct chunk *chk, struct allocator_base *base)
{
	chk_next(chk) = chk_head(base);
	chk_head(base) = chk;
}

static struct chunk *chunk_new(int k, int metasize)
{
	struct chunk *chk = rt_malloc(N1024 * k);
	if (!chk)
		return NULL;
	chk->size = N1024 * k - sizeof(struct chunk);
	chk_next(chk) = NULL;
	int align = ((size_t)chk->mem + metasize) & (BLK_BASE - 1);
	chk->pos = align ? BLK_BASE - align : 0;
	return chk;
}

static struct chunk *chunk_pickup(struct allocator_base *base, int size)
{
	struct chunk *prev = NULL;
	struct chunk *chk = chk_head(base);
	while (chk) {
		if (chk->pos + size <= chk->size)
			break;
		prev = chk;
		chk = chk_next(chk);
	}
	if (!chk) {
		int chksize = size + (sizeof(struct chunk) + BLK_BASE + (N1024 - 1));
		chk = chunk_new(chksize / N1024 <= base->chksize ? base->chksize : chksize / N1024, base->metasize);
		if (chk)
			chunk_add(chk, base);
	} else if (prev) {
		// moves current "chk" to top
		chk_next(prev) = chk_next(chk);
		chunk_add(chk, base);
	}
	return chk;
}

static void freelist_reset(void **freelist, int max)
{
	for (int i = 0; i < max; i++)
		freelist[i] = NULL;
}

static inline int freelist_index(unsigned int size)
{
	int i = size / BLK_BASE - 1;
	return i < FREELIST_MAX ? i : FREELIST_MAX;
}

static struct meta *freelist_get(struct tinyalloc_root *root, int size)
{
	int i = freelist_index(size);
	struct meta *curr = FREE_HEAD(root, i);
	if (curr && i < FREELIST_MAX) {
		FREE_HEAD(root, i) = FREE_NEXT(curr);
		return curr;
	}
	struct meta *prev = NULL;
	while (curr) {
		int full = META_FULLSIZE(curr);
		if (full < size) {
			prev = curr;
			curr = FREE_NEXT(curr);
			continue;
		}
		if (full >= size + (BLK_BASE * (int)ARRAYSIZE(root->freelist))) { // Do Splits
			struct meta *next = (struct meta *)((char *)curr + size);
			META_DATASIZE(curr) = size - sizeof(struct meta);
			META_DATASIZE(next) = full - sizeof(struct meta) - size;
			FREE_NEXT(next) = FREE_NEXT(curr);
			FREE_NEXT(curr) = next;
		}
		if (prev) {
			FREE_NEXT(prev) = FREE_NEXT(curr);
		} else {
			FREE_HEAD(root, i) = FREE_NEXT(curr);
		}
		break;
	}
	return curr;
}

void tinyalloc_init(struct tinyalloc_root *root, int chksize)
{
	if (chksize < 8)
		chksize = 8;
	root->base = (struct allocator_base){
		.chksize = chksize,
		.metasize = sizeof(struct meta),
		.chunk_head = NULL
	};
	FREE_RESET(root->freelist);
}

void *tinyalloc(struct tinyalloc_root *root, int size)
{
	if (size < (16 - sizeof(struct meta))) {
		size = 16;
	} else {
		size = ALIGN_POW2(size + sizeof(struct meta), BLK_BASE);
	}
	struct meta *meta = freelist_get(root, size);
	if (meta)
		return META_DATAPTR(meta);

	struct chunk *chk = chunk_pickup(the_base(root), size);
	if (!chk)
		return NULL;
	meta = (struct meta *)chk_dataptr(chk);
	META_DATASIZE(meta) = size - sizeof(struct meta);
	chk->pos += size;
	return META_DATAPTR(meta);
}

void tinyfree(struct tinyalloc_root *root, void *ptr)
{
	if (NOT_ALIGNED((size_t)ptr, BLK_BASE))
		return;
	struct meta *meta = container_of(ptr, struct meta, data);
	int i = freelist_index(META_FULLSIZE(meta));
	FREE_NEXT(meta) = FREE_HEAD(root, i);
	FREE_HEAD(root, i) = meta;
}

static void chunks_reset(struct allocator_base *base)
{
	struct chunk *chk = chk_head(base);
	while (chk) {
		int align = ((size_t)chk->mem + base->metasize) & (BLK_BASE - 1);
		chk->pos = align ? BLK_BASE - align : 0;
		chk = chk_next(chk);
	}
}

static void chunks_destroy(struct allocator_base *base)
{
	struct chunk *chk = chk_head(base);
	struct chunk *next;
	while (chk) {
		next = chk_next(chk);
		rt_free(chk);
		chk = next;
	}
	chk_head(base) = NULL;
}

void tinyreset(struct tinyalloc_root *root)
{
	chunks_reset(the_base(root));
	FREE_RESET(root->freelist);
}

void tinydestroy(struct tinyalloc_root *root)
{
	chunks_destroy(the_base(root));
	FREE_RESET(root->freelist);
}

/**
*
* bump allocator
*
*/
void bumpalloc_init(struct bumpalloc_root *bump, int chksize)
{
	if (chksize <= 0)
		chksize = 1;
	bump->base = (struct allocator_base){
		.chksize = chksize,
		.metasize = 0,
		.chunk_head = NULL
	};
}

void *bumpalloc(struct bumpalloc_root *bump, int size)
{
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	struct chunk *chk = chunk_pickup(the_base(bump), size);
	if (!chk)
		return NULL;
	char *ptr = chk_dataptr(chk);
	chk->pos += size;
	return ptr;
}

void bumpreset(struct bumpalloc_root *bump)
{
	chunks_reset(the_base(bump));
}

void bumpdestroy(struct bumpalloc_root *bump)
{
	chunks_destroy(the_base(bump));
}


/**
*
* fixed allocator
*
*/
#define FIXED_NEXT(ptr)   (*(void **)(ptr))
#define FIXED_HEAD(fixed) ((fixed)->freelist[0])

void fixedalloc_init(struct fixedalloc_root *fixed, int chksize, int size)
{
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	fixed->size = size;

	if (chksize <= 0)
		chksize = 1;
	fixed->base = (struct allocator_base){
		.chksize = chksize,
		.metasize = 0,
		.chunk_head = NULL
	};
	FIXED_HEAD(fixed) = NULL;
}

void *fixedalloc(struct fixedalloc_root *fixed)
{
	void *result = FIXED_HEAD(fixed);
	if (result) {
		FIXED_HEAD(fixed) = FIXED_NEXT(result);
		return result;
	}
	const int size = fixed->size;
	struct chunk *chk = chunk_pickup(the_base(fixed), size);
	if (!chk)
		return NULL;
	result = chk_dataptr(chk);
	chk->pos += size;

	// pre-allocation
	char *ptr = chk_dataptr(chk);
	for (int i = 0; i < 32; i++) {
		if (chk->pos + size > chk->size)
			break;
		FIXED_NEXT(ptr) = FIXED_HEAD(fixed);
		FIXED_HEAD(fixed) = ptr;
		ptr += size;
		chk->pos += size;
	}
	return result;
}

void fixedfree(struct fixedalloc_root *fixed, void *ptr)
{
	if (NOT_ALIGNED((size_t)ptr, BLK_BASE))
		return;
	FIXED_NEXT(ptr) = FIXED_HEAD(fixed);
	FIXED_HEAD(fixed) = ptr;
}

void fixedreset(struct fixedalloc_root *fixed)
{
	chunks_reset(the_base(fixed));
	FIXED_HEAD(fixed) = NULL;
}

void fixeddestroy(struct fixedalloc_root *fixed)
{
	chunks_destroy(the_base(fixed));
	FIXED_HEAD(fixed) = NULL;
}
