/*
* SPDX-License-Identifier: GPL-2.0
*/

#include <stdlib.h>
#include <string.h>
#include "tinyalloc.h"
#include "slist.h"

#define CHK_SIZE           TINYALLOC_CHK_SIZE
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
#define FREE_ROOT(root, i) ((root)->freelist[i])
#define N1024              1024

struct chunk {
	int pos;
	int size; // sizeof(chunk->mem)
	union {
		struct slist_head link;
		double __x; // align(struct chunk) to 8bytes if compiler is 32bit
	};
	char mem[0];
};

#define chk_to_node(chk)   (&(chk)->link)
#define chk_from_node(n)   slist_entry(n, struct chunk, link)
#define chk_next(chk)      slist_next(chk_to_node(chk))
#define chk_root(root)     (&(root)->chunk_head)
#define chk_dataptr(chk)   ((chk)->mem + (chk)->pos)

static struct chunk *chunk_new(int k, int metasize)
{
	struct chunk *chk = malloc(N1024 * k);
	if (!chk)
		return NULL;
	chk->size = N1024 * k - sizeof(struct chunk);
	chk->link.next = NULL;
	int align = ((size_t)chk->mem + metasize) & (BLK_BASE - 1);
	chk->pos = align ? BLK_BASE - align : 0;
	return chk;
}

static struct chunk *chunk_pickup(struct slist_head *head, int size, int metasize)
{
	int chksize;
	struct chunk *chk;
	struct chunk *prev = NULL;
	if (slist_first(head)) {
		chk = chk_from_node(slist_first(head));
	} else {
		goto Chunknew;
	}
	while (true) {
		if (chk->pos + size <= chk->size)
			break;

		if (chk_next(chk)) {
			prev = chk;
			chk = chk_from_node(chk_next(chk));
			continue;
		}
	Chunknew:
		chksize = size + (sizeof(struct chunk) + BLK_BASE + (N1024 - 1));
		chk = chunk_new(chksize / N1024 <= CHK_SIZE ? CHK_SIZE : chksize / N1024, metasize);
		if (chk)
			slist_add(chk_to_node(chk), head);
		return chk;
	}
	if (prev) { // moves current "chk" to top
		chk_next(prev) = chk_next(chk);
		slist_add(chk_to_node(chk), head);
	}
	return chk;
}

static inline int freelist_index(unsigned int size)
{
	int i = size / BLK_BASE - 1;
	return i < FREELIST_MAX ? i : FREELIST_MAX;
}

static struct meta *freelist_get(struct tinyalloc_root *root, int size)
{
	int i = freelist_index(size);
	struct meta *curr = FREE_ROOT(root, i);
	if (curr && i < FREELIST_MAX) {
		FREE_ROOT(root, i) = FREE_NEXT(curr);
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
			FREE_ROOT(root, i) = FREE_NEXT(curr);
		}
		break;
	}
	return curr;
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

	struct chunk *chk = chunk_pickup(chk_root(root), size, sizeof(struct meta));
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
	FREE_NEXT(meta) = FREE_ROOT(root, i);
	FREE_ROOT(root, i) = meta;
}

static void chunks_reset(struct slist_head *head, int metasize)
{
	struct chunk *chk;
	struct slist_head *node;
	slist_for_each(node, head) {
		chk = chk_from_node(node);
		int align = ((size_t)chk->mem + metasize) & (BLK_BASE - 1);
		chk->pos = align ? BLK_BASE - align : 0;
	}
}

static void chunks_destroy(struct slist_head *head)
{
	struct chunk *chk;
	while (slist_first(head)) {
		chk = chk_from_node(slist_pop(head));
		free(chk);
	}
}

static void freelist_reset(void **freelist, int max)
{
	for (int i = 0; i < max; i++)
		freelist[i] = NULL;
}

void tinyreset(struct tinyalloc_root *root)
{
	chunks_reset(chk_root(root), sizeof(struct meta));
	freelist_reset(root->freelist, ARRAYSIZE(root->freelist));
}

void tinydestroy(struct tinyalloc_root *root)
{
	chunks_destroy(chk_root(root));
	freelist_reset(root->freelist, ARRAYSIZE(root->freelist));
}

/**
*
* bump allocator
*
*/
void *bumpalloc(struct bumpalloc_root *bump, int size)
{
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	struct chunk *chk = chunk_pickup(chk_root(bump), size, 0);
	if (!chk)
		return NULL;
	char *ptr = chk_dataptr(chk);
	chk->pos += size;
	return ptr;
}

void bumpreset(struct bumpalloc_root *bump)
{
	chunks_reset(chk_root(bump), 0);
}

void bumpdestroy(struct bumpalloc_root *bump)
{
	chunks_destroy(chk_root(bump));
}


/**
*
* fixed allocator
*
*/
#define FIXED_NEXT(ptr)   (*(void **)(ptr))
#define FIXED_HEAD(fixed) ((fixed)->freelist[0])

void fixedalloc_init(struct fixedalloc_root *fixed, int size)
{
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	INIT_SLIST_HEAD(chk_root(fixed));
	FIXED_HEAD(fixed) = NULL;
	fixed->size = size;
}

void *fixedalloc(struct fixedalloc_root *fixed)
{
	void *result = FIXED_HEAD(fixed);
	if (result) {
		FIXED_HEAD(fixed) = FIXED_NEXT(result);
		return result;
	}
	const int size = fixed->size;
	struct chunk *chk = chunk_pickup(chk_root(fixed), size, 0);
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
	chunks_reset(chk_root(fixed), 0);
	FIXED_HEAD(fixed) = NULL;
}

void fixeddestroy(struct fixedalloc_root *fixed)
{
	chunks_destroy(chk_root(fixed));
	FIXED_HEAD(fixed) = NULL;
}
