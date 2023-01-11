/*
* SPDX-License-Identifier: GPL-2.0
*/

#ifndef R32_TINYALLOC_H
#define R32_TINYALLOC_H
#include "comm.h"

#ifndef TINYALLOC_CHK_SIZE
	#define TINYALLOC_CHK_SIZE          64
#endif
#ifndef TINYALLOC_BLK_BASE
	#define TINYALLOC_BLK_BASE          8
#endif
#ifndef TINYALLOC_FREELIST_MAX
	#define TINYALLOC_FREELIST_MAX      16
#endif

struct tinyalloc_root {
	struct slist_head chunk_head;
	void *freelist[TINYALLOC_FREELIST_MAX + 1];
};

struct bumpalloc_root {
	struct slist_head chunk_head;
};

C_FUNCTION_BEGIN

void tinyfree(struct tinyalloc_root *root, void *ptr);

void *tinyalloc(struct tinyalloc_root *root, int size);

void tinyreset(struct tinyalloc_root *root);

void tinydestroy(struct tinyalloc_root *root);

// bump alloctor
void *bumpalloc(struct bumpalloc_root *bump, int size);

void bumpreset(struct bumpalloc_root *bump);

void bumpdestroy(struct bumpalloc_root *bump);

C_FUNCTION_END
#endif
