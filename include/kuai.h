/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#ifndef LWM_KUAI_H
#define LWM_KUAI_H

#define KFREELIST_MAX 15

/*
 * 'kuai' - A lightweight first-class heaps allocator.
 *
 * Memory Layout :
 * [Root]
 *   |
 *   +- slab(64KB) -> slab(64KB) -> ... -> NULL  (Linked list of slabs)
 *   |     |
 *   |     +- pos  : Current allocation position in the slab
 *   |     +- flag : 0 = externally allocated(when allocation size > 8KB).
 *   |     +- next : Pointer to next slab
 *   |     +- 1KB bitmap : Tracks block status (1 bit per BLK_BASE(8 bytes))
 *   |
 *   +- freelist : Released blocks are linked here during kuai_free()
 *         |
 *         +  [0] : (pmap root) : AVL tree for free blocks [MAX ~ 8KB]
 *         +- [1] : list(1 * BLK_BASE)
 *         +- [2] : list(2 * BLK_BASE)
 *         +- ...
 *         +- [MAX - 1] : list((MAX - 1) * BLK_BASE)
 *
 * TODO : A better way to handle >8KB blocks.
 */
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
