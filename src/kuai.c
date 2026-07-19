/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "kuai.h"
#include "pmap.h"

// Round 'size' up to a multiple of 'alignment' (must be a power of two).
// This macro ensures 'size' is evaluated **only once** to avoid some issue.
#ifndef ALIGN_UP
#   define ALIGN_UP(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#endif

#ifndef IS_64
#   define IS_64         (SIZE_MAX == UINT64_MAX)
#endif
#if IS_64
#   define BIT_CNT_MAX   (64)
#else
#   define BIT_CNT_MAX   (32)
#endif
#define BIT_SHIFT_MAX    (BIT_CNT_MAX - 1)


#if defined(_MSC_VER) || defined(__MSVCRT__)
#   include <intrin.h>
#   if IS_64
#       define bit_scan_forward _BitScanForward64
#   else
#       define bit_scan_forward _BitScanForward
#   endif
static unsigned int __inline TRAILING_ZEROS(size_t x)
{
	unsigned long msb;
	if(bit_scan_forward(&msb, x))
		return msb;
	return BIT_CNT_MAX;
}
#undef bit_scan_forward
#else
#   if IS_64
#       define trailing_zeros __builtin_ctzll
#   else
        define trailing_zeros __builtin_ctz
#   endif
static inline unsigned int TRAILING_ZEROS(size_t x)
{
	return x ? trailing_zeros(x) : BIT_CNT_MAX;
}
#undef trailing_zeros
#endif

// Fallback threshold, If alloc size > this threshold, use external allocator
#define EXTERN_SIZE               (8 * 1024)

// The bytes size of the basic block, the bitmap uses 1 bit to represent each BLK_BASE.
#ifndef BLK_BASE
#define BLK_BASE                  (8)
#endif

// The number of bytes managed by one bitmap byte
#define BMPBYTE_CBS               (BLK_BASE * CHAR_BIT)

// The number of bytes managed by one size_t-sized bitmap word
#define BMPLONG_CBS               (BMPBYTE_CBS * sizeof(size_t))

// byte to bitmap index
#define BMPLONG_INDEX(p)          ((p) / BMPLONG_CBS)

// bit position in [0-31] or [0-63]
#define BMPLONG_BIT_POSITION(p)   (((p) % BMPLONG_CBS) / BLK_BASE)

// The total memory size of a slab (in bytes)
#if (BLK_BASE == 8)
#   define SLAB_SIZE              (64 * 1024)
#elif (BLK_BASE == 16)
#   define SLAB_SIZE              (256 * 1024)
#endif

// The bitmap size in bytes required to track
#define BMPBYTE_SIZE              (SLAB_SIZE / BMPBYTE_CBS)

// The bitmap size in size_t units
#define BMPLONG_SIZE              (BMPBYTE_SIZE / sizeof(size_t))

// The position start offset
#define POSITION_START            (BMPBYTE_SIZE)

// The minimum bytes size required for (struct slab.meta.stat)
#define STAT_SIZE                 16

#if ((POSITION_START / BMPBYTE_CBS) < STAT_SIZE)
#   error "(POSITION_START / BMPBYTE_CBS) needs to be at least 16 bytes."
#endif

#define BPTR(p)                   ((unsigned char *)(p))

#define COMMIT_PAGE               (8 * 1024)
#if defined(_MSC_VER) || defined(__MSVCRT__)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   define VALLOC(size)            VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE)
#   define VCOMMIT(ptr, size)      VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE)
#   define VRESET(ptr, size)       VirtualAlloc(ptr, size, MEM_RESET, PAGE_NOACCESS)
#   define VFREE(ptr)              VirtualFree(ptr, 0, MEM_RELEASE)
#else
#include <sys/mman.h>
#   define VALLOC(size)            mmap(NULL, size, PROT_NONE             , MAP_PRIVATE | MAP_ANONYMOUS            , -1, 0)
#   define VCOMMIT(ptr, size)      mmap(ptr , size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0)
#   define VRESET(ptr, size)       madvise(ptr, size, MADV_DONTNEED)
#   define VFREE(ptr)              munmap(ptr, SLAB_SIZE)
#endif

/*
 * The bitmap uses 1 bit to represent each BLK_BASE.
 * 1 : indicates the start of the BLOCK
 * 0 : indicates the contiguous BLOCK
 */
struct slab {
	union {
		struct {
			int pos;
			int committed; // 0 means allocated by external alloctor
			struct slab *next;
		} stat;
		size_t bitmap[BMPLONG_SIZE];
	} meta;
	unsigned char data[SLAB_SIZE - BMPBYTE_SIZE];
};

#define slab_head(r)      ((r)->slab)
#define slab_next(p)      ((p)->meta.stat.next)
#define slab_pos(p)       ((p)->meta.stat.pos)
#define slab_committed(p) ((p)->meta.stat.committed)
#define slab_isinner(p)   ((p)->meta.stat.committed)

#define FREE_NEXT(m)      (*(void **)(m))
#define FREE_HEAD(k, i)   ((k)->freelist[i])

static inline void slab_init(struct slab *slab)
{
	slab_committed(slab) = COMMIT_PAGE;
	// starting point
	slab_pos(slab) = POSITION_START;
	//
	slab_next(slab) = NULL;
	// mark the first block as valid for block_validate
	slab->meta.bitmap[BMPLONG_INDEX(POSITION_START)] = 1;
}

static inline void slab_add(struct kuai *kuai, struct slab *slab)
{
	slab_next(slab) = slab_head(kuai);
	slab_head(kuai) = slab;
}
static struct slab *slab_new()
{
	struct slab *slab = VALLOC(SLAB_SIZE);
	if (!slab)
		return NULL;
	// incremental commit in MSVC
	VCOMMIT(slab, COMMIT_PAGE);
	slab_init(slab);
	return slab;
}

static int block_size(struct slab *slab, void *block)
{
	int offset = (int)(BPTR(block) - BPTR(slab));
	int index = BMPLONG_INDEX(offset);
	int begin = BMPLONG_BIT_POSITION(offset);

	size_t bits = slab->meta.bitmap[index++];
	//if (((size_t)1 << begin & bits) == 0)
	//	return 0;
	// count bits starting at the next bit point
	if (begin < BIT_SHIFT_MAX) {
		bits >>= begin + 1;
	} else if (index < BMPLONG_SIZE) {
		bits = slab->meta.bitmap[index++];
		begin = -1; // prev begin
	} else {
		return BLK_BASE;
	}
	if (bits)
		return (1 + TRAILING_ZEROS(bits)) * BLK_BASE;
	int cnt = BIT_CNT_MAX - begin;
	while (index < BMPLONG_SIZE) {
		bits = slab->meta.bitmap[index++];
		if (bits) {
			cnt += TRAILING_ZEROS(bits);
			break;
		}
		cnt += BIT_CNT_MAX;
	}
	return cnt * BLK_BASE;
}

static void block_split(struct slab *slab, void *block)
{
	int offset = (int)(BPTR(block) - BPTR(slab));
	int index = BMPLONG_INDEX(offset);
	int begin = BMPLONG_BIT_POSITION(offset);
	size_t mask = (size_t)1 << begin;
	slab->meta.bitmap[index] |= mask;
}

static int block_validate(struct slab *slab, void *block)
{
	int offset = (int)(BPTR(block) - BPTR(slab));
	int index = BMPLONG_INDEX(offset);
	int begin = BMPLONG_BIT_POSITION(offset);
	size_t mask = (size_t)1 << begin;
	return (slab->meta.bitmap[index] & mask) != 0;
}

static void *block_add(struct slab *slab, int size)
{
	int offset = slab_pos(slab);
	void *block = BPTR(slab) + offset;
	int committed = slab_committed(slab);
	int next = offset + size;
	slab_pos(slab) = next; // assign before return
	int grow = next - committed;
	if (grow >= 0) {
		if (next == SLAB_SIZE)
			return block;
		int page = grow ? ALIGN_UP(grow, COMMIT_PAGE) : COMMIT_PAGE;
		VCOMMIT(BPTR(slab) + committed, page);
		slab_committed(slab) = committed + page;
	}
	// Just simply mark the next point as a separator.
	int index = BMPLONG_INDEX(next);
	int begin = BMPLONG_BIT_POSITION(next);
	slab->meta.bitmap[index] |= (size_t)1 << begin;
	return block;
}

static inline int free_index(unsigned int size)
{
	int i = size / BLK_BASE;
	return i < KFREELIST_MAX ? i : 0;
}

/*
 * freed blocks of varying sizes, for size >= KFREELIST_MAX
 */
struct vblock {
	struct pmnode node;
	struct pmnode *next;
	int size;
};
// sizeof(struct vblock) / BLK_SIZE ==> 40 / 8
#if (KFREELIST_MAX < 5)
#   error "The value of KFREELIST_MAX is below 5"
#endif
#define PMAP_ROOT(k)     ((struct pmnode **)&(k)->freelist[0])
#define VBLOCK_SIZE(b)   ((b)->size)

static struct vblock *vblock_remove(struct pmnode **root, int size)
{
	int index = -1;
	int lower = -1; // lower bound
	struct pmnode **slot = root;
	struct pmnode **stacks[PMAP_STACK_HEIGHT];
	while (*slot) {
		struct pmnode *pnode = *slot;
		int cmp = size - VBLOCK_SIZE(container_of(pnode, struct vblock, node));
		if (cmp == 0)
			break;
		stacks[++index] = slot;
		if (cmp < 0) {
			lower = index;
			slot = &pnode->left;
		} else {
			slot = &pnode->right;
		}
	}
	struct pmnode *victim = *slot;
	if (victim == NULL) {
		if (lower < 0)
			return NULL;
		slot = stacks[lower];
		victim = *slot;
		index = lower - 1;
	}
	struct vblock *vblock = container_of(victim, struct vblock, node);
	if (vblock->next) {
		struct pmnode *next = vblock->next;
		*next = vblock->node; // copy metadata
		*slot = next;         // linking
		return vblock;
	}
	// remove slot and do balance
	pmap_remove(slot, stacks, index);

	return vblock;
}
static void vblock_upsert(struct pmnode **root, struct vblock *vblock, int size)
{
	int index = -1;
	struct pmnode **slot = root;
	struct pmnode **stacks[PMAP_STACK_HEIGHT];
	VBLOCK_SIZE(vblock) = size;
	while (*slot) {
		struct pmnode *pnode = *slot;
		int cmp = size - VBLOCK_SIZE(container_of(pnode, struct vblock, node));
		stacks[++index] = slot;
		if (cmp < 0) {
			slot = &pnode->left;
		} else if (cmp > 0) {
			slot = &pnode->right;
		} else {
			vblock->node = *pnode; // copy metadata
			vblock->next = pnode;
			*slot = &vblock->node; // linking
			return;
		}
	}
	// init for new one
	vblock->node = (struct pmnode){ .left = NULL, .right = NULL, .height = 1 };
	vblock->next = NULL;

	// link node to the NULL place.
	*slot = &vblock->node;

	// do balance start at the slot's parent
	pmap_balance(stacks, index);
}

static void *free_pickup(struct kuai *kuai, int size)
{
	int i = free_index(size);
	if (i) {
		void *block = FREE_HEAD(kuai, i);
		if (block) {
			FREE_HEAD(kuai, i) = FREE_NEXT(block);
		}
		return block;
	}
	struct vblock *vblock = vblock_remove(PMAP_ROOT(kuai), size);
	if (vblock && VBLOCK_SIZE(vblock) >= size + (BLK_BASE * KFREELIST_MAX)) { // Splits
		// linear search the owner
		struct slab **prev = &slab_head(kuai);
		struct slab *slab = NULL;
		while (slab = *prev) {
			if (BPTR(vblock) > BPTR(slab) && BPTR(vblock) < BPTR(slab) + SLAB_SIZE)
				break;
			prev = &slab_next(slab);
		}
		struct vblock *next = (struct vblock *) (BPTR(vblock) + size);
		block_split(slab, next);
		vblock_upsert(PMAP_ROOT(kuai), next, VBLOCK_SIZE(vblock) - size);
	}
	return vblock;
}

void kuai_init(struct kuai *kuai)
{
	*kuai = (struct kuai){ 0 };
	slab_head(kuai) = slab_new();
}

void kuai_destroy(struct kuai *kuai)
{
	struct slab *slab = slab_head(kuai);
	struct slab *next;
	while (slab) {
		next = slab_next(slab);
		if (slab_isinner(slab)) {
			VFREE(slab);
		} else {
			free(slab);
		}
		slab = next;
	}
	*kuai = (struct kuai){ 0 };
}

/*
 * Try to keep only one slab and then reset it
 */
void kuai_reset(struct kuai *kuai)
{
	struct slab *slab = slab_head(kuai);
	// exclude external slab at the begining
	while (slab && !slab_isinner(slab)) {
		struct slab *next = slab_next(slab);
		free(slab);
		slab = next;
	}
	slab_head(kuai) = slab ? slab_next(slab) : NULL;
	kuai_destroy(kuai);
	if (!slab)
		return;

	if (slab_committed(slab) > COMMIT_PAGE)
		VRESET(slab + COMMIT_PAGE, slab_committed(slab) - COMMIT_PAGE);
	memset(slab, 0, COMMIT_PAGE);
	slab_init(slab);
	slab_head(kuai) = slab;
}

void *kuai_alloc(struct kuai *kuai, int size)
{
	// if size > 8KB
	if (size > EXTERN_SIZE) {
		size += STAT_SIZE;
		struct slab *slab = malloc(size);
		if (!slab)
			return NULL;
		slab_pos(slab) = size;
		slab_isinner(slab) = 0;
		slab_next(slab) = NULL;
		// Add to tail
		struct slab **prev = &slab_head(kuai);
		while (*prev)
			prev = &slab_next(*prev);
		*prev = slab;
		return BPTR(slab) + STAT_SIZE;
	}
	size = size ? ALIGN_UP(size, BLK_BASE) : BLK_BASE;
	void *block = free_pickup(kuai, size);
	if (block)
		return block;
	struct slab *prev = NULL;
	struct slab *slab = slab_head(kuai);
	while (slab) {
		if (SLAB_SIZE - slab_pos(slab) >= size && slab_isinner(slab))
			break;
		prev = slab;
		slab = slab_next(slab);
	}
	if (!slab) {
		slab = slab_new();
		if (!slab)
			return NULL;
		slab_add(kuai, slab);
	}
	return block_add(slab, size);
}

void kuai_free(struct kuai *kuai, void *block)
{
	struct slab **prev = &slab_head(kuai);
	struct slab *slab = NULL;
	while (slab = *prev) {
		if (BPTR(block) > BPTR(slab) && BPTR(block) < BPTR(slab) + slab_pos(slab))
			break;
		prev = &slab_next(slab);
	}
	if (!slab)
		return;
	// external alloc
	if (!slab_isinner(slab)) {
		*prev = slab_next(slab);
		free(slab);
		return;
	}
	if (!block_validate(slab, block))
		return;
	int size = block_size(slab, block);
	int i = free_index(size);
	if (i) {
		FREE_NEXT(block) = FREE_HEAD(kuai, i);
		FREE_HEAD(kuai, i) = block;
	} else {
		vblock_upsert(PMAP_ROOT(kuai), block, size);
	}
}
