/*
 *  SPDX-License-Identifier: GPL-2.0
 *
 * 'kuai' - A lightweight first-class heaps allocator
 */

#ifndef LWM_KUAI_H
#define LWM_KUAI_H

#define KFREELIST_MAX 15
struct kuai {
	void *slab;
	void *freelist[KFREELIST_MAX];
};

void kuai_init(struct kuai *kuai);
void kuai_reset(struct kuai *kuai);
void kuai_destroy(struct kuai *kuai);

void *kuai_alloc(struct kuai *kuai, int size);
void kuai_free(struct kuai *kuai, void *block);

#endif
