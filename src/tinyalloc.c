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
#define chk_add(chk, root) slist_add(chk_to_node(chk), chk_root(root))

static struct chunk *chk_new(int k)
{
	struct chunk *chk = malloc(N1024 * k);
	if (!chk)
		return NULL;
	chk->size = N1024 * k - sizeof(struct chunk);
	chk->link.next = NULL;
	int align = ((size_t)chk->mem + sizeof(struct meta)) & (BLK_BASE - 1);
	if (align)
		align = BLK_BASE - align;
	chk->pos = align;
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
		if (full >= size + (BLK_BASE * (FREELIST_MAX + 1))) { // Do Splits
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

	struct chunk *chk;
	struct chunk *prev = NULL;
	if (slist_empty(chk_root(root))) {
		chk = chk_new(CHK_SIZE);
		if (chk == NULL)
			return NULL;
		chk_add(chk, root);
	} else {
		chk = slist_first_entry(chk_root(root), struct chunk, link);
	}
	while (true) {
		if (chk->pos + size <= chk->size) {
			meta = (struct meta *)(chk->mem + chk->pos);
			META_DATASIZE(meta) = size - sizeof(struct meta);
			chk->pos += size;
			break;
		}
		if (chk_next(chk)) {
			prev = chk;
			chk = chk_from_node(chk_next(chk));
		} else {
			int chksize = size + (sizeof(struct chunk) + BLK_BASE + (N1024 - 1));
			chk = chk_new(chksize / N1024 <= CHK_SIZE ? CHK_SIZE : chksize / N1024);
			if (chk == NULL)
				return NULL;
			prev = NULL;
			chk_add(chk, root);
		}
	}
	// moves current "chk" to top
	if (prev) {
		chk_next(prev) = chk_next(chk);
		chk_add(chk, root);
	}
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

void tinyreset(struct tinyalloc_root *root)
{
	struct chunk *chk;
	struct slist_head *pos;
	slist_for_each(pos, chk_root(root)) {
		chk = chk_from_node(pos);
		int align = ((size_t)chk->mem + sizeof(struct meta)) & (BLK_BASE - 1);
		if (align)
			align = BLK_BASE - align;
		chk->pos = align;
	}
	int i = 0;
	while (i < (FREELIST_MAX + 1)) {
		FREE_ROOT(root, i++) = NULL;
	}
}

void tinydestroy(struct tinyalloc_root *root)
{
	struct chunk *chk;
	struct slist_head *head = chk_root(root);
	while (slist_first(head)) {
		chk = chk_from_node(slist_pop(head));
		free(chk);
	}
	int i = 0;
	while (i < (FREELIST_MAX + 1)) {
		FREE_ROOT(root, i++) = NULL;
	}
}
