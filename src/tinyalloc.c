/*
* SPDX-License-Identifier: GPL-2.0
*/

#include <stdlib.h>
#include <string.h>
#include "tinyalloc.h"
#include "slist.h"

struct meta {
	int datasize;
	void* data[0];
};

static inline int meta_size(struct meta* m) { return sizeof(struct meta) + m->datasize; }

#define META_DATABPTR(m)  ((unsigned char*)&(m)->data)
#define META_DATASIZE(m)  ((m)->datasize)
#define FREE_NEXT(m)      ((m)->data[0])
#define FREE_ROOT(chk, i) ((chk)->freelist[i])
#define N1024             1024
#define FREELIST_MAX      8
#define BLK_BASE          8
#ifndef K_CHKSIZE
#   define K_CHKSIZE      64
#endif

struct chunk {
	int mempos;
	int memsize;
	int frags;
	int blocks;
	struct meta* freelist[FREELIST_MAX + 1];
	struct slist_head link;
	char mem[0];
};

#define pos_next(chk) slist_next(&(chk)->link)
#define chk_add(chk, root) slist_add(&(chk)->link, root)
#define chk_entry(pos) slist_entry(pos, struct chunk, link)

static inline char* chk_posptr(struct chunk* chk) { return chk->mem + chk->mempos; }

static struct chunk* chk_new(int k) {
	struct chunk* chk = malloc(N1024 * k);
	if (chk) {
		memset(chk, 0, sizeof(struct chunk));
		chk->memsize = N1024 * k - sizeof(struct chunk);
	}
	return chk;
}

static inline void chk_reset(struct chunk* chk) {
	for (int i = 0; i < (FREELIST_MAX + 1); i++) {
		FREE_ROOT(chk, i) = NULL;
	}
	chk->frags = 0;
	chk->blocks = 0;
	chk->mempos = 0;
}

static inline int freelist_pos(unsigned int mul) {
	int i = mul / BLK_BASE - 1;
	return i < FREELIST_MAX ? i : FREELIST_MAX;
}

static struct meta* freelist_get(struct chunk* chk, int size) {
	int i = freelist_pos(size);
	struct meta* curr = FREE_ROOT(chk, i);
	if (curr && i < FREELIST_MAX) {
		FREE_ROOT(chk, i) = FREE_NEXT(curr);
		return curr;
	}
	struct meta* prev = NULL;
	while(curr) {
		int rest = META_DATASIZE(curr) - size;
		if (rest < 0) {
			prev = curr;
			curr = FREE_NEXT(curr);
			continue;
		}
		rest -= sizeof(struct meta);
		if (rest >= size) { // Do Split
			struct meta* next = (struct meta*)(META_DATABPTR(curr) + size);
			META_DATASIZE(curr) = size;
			META_DATASIZE(next) = rest;
			chk->blocks++;
			chk->frags++;
			FREE_NEXT(next) = FREE_NEXT(curr);
			FREE_NEXT(curr) = next;
		}
		if (prev) {
			FREE_NEXT(prev) = FREE_NEXT(curr);
		} else {
			FREE_ROOT(chk, i) = FREE_NEXT(curr);
		}
		break;
	}
	return curr;
}

void* tinyalloc(struct slist_head* root, int size) {
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else if (NOT_ALIGNED(size, BLK_BASE)) {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	struct chunk* chk = NULL;
	if (slist_empty(root)) {
		chk = chk_new(K_CHKSIZE);
		if (chk == NULL)
			return NULL;
		chk_add(chk, root);
	} else {
		chk = slist_first_entry(root, struct chunk, link);
	}
	struct meta* meta;
	struct chunk* prev = NULL;
	int needs = sizeof(struct meta) + size;
	do {
		meta = freelist_get(chk, size);
		if (meta) {
			chk->frags--;
			break;
		}
		if (chk->mempos + needs <= chk->memsize) {
			meta = (struct meta*)chk_posptr(chk);
			META_DATASIZE(meta) = size;
			chk->mempos += needs;
			chk->blocks++;
			break;
		}
		if (pos_next(chk)) {
			prev = chk;
			chk = chk_entry(pos_next(chk));
		} else {
			int chksize = needs + (sizeof(struct chunk) + (N1024 - 1)); // since (1025 + 1023) / 1024 == 2
			chk = chk_new(chksize / N1024 <= K_CHKSIZE ? K_CHKSIZE : chksize / N1024);
			if (chk == NULL)
				return NULL;
			prev = NULL;
			chk_add(chk, root);
		}
	} while(chk);
	// moving current "chk" at top of the list when prev exists
	if (prev) {
		pos_next(prev) = pos_next(chk);
		chk_add(chk, root);
	}
	return META_DATABPTR(meta);
}

#define INTERVAL(meta, chk) \
	((char*)(meta) >= (chk)->mem && (char*)(meta) + meta_size(meta) <= chk_posptr(chk))

void tinyfree(struct slist_head* root, void* ptr) {
	struct chunk* chk;
	struct slist_head* pos;
	struct meta* meta = container_of(ptr, struct meta, data);
	slist_for_each(pos, root) {
		chk = chk_entry(pos);
		if (INTERVAL(meta, chk))
			break;
	}
	if (pos == NULL) // init by slist_for_each
		return;
	chk->frags++;
	// add to freelist
	int i = freelist_pos( META_DATASIZE(meta) );
	FREE_NEXT(meta) = FREE_ROOT(chk, i);
	FREE_ROOT(chk, i) = meta;

	// simply reset
	if (chk->frags == chk->blocks) {
		chk_reset(chk);
	}
}

void tinyreset(struct slist_head* root) {
	struct chunk* chk;
	struct slist_head* pos;
	slist_for_each(pos, root) {
		chk = chk_entry(pos);
		chk_reset(chk);
	}
}

void tinydestroy(struct slist_head* root) {
	struct chunk* chk;
	while(slist_first(root)) {
		chk = chk_entry(slist_pop(root));
		free(chk);
	}
}
