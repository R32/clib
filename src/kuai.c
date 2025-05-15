/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "kuai.h"

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


#if defined(_MSC_VER)
#   include <intrin.h>
#   if IS_64
#       define bit_scan_forward _BitScanForward64
#   else
        define bit_scan_forward _BitScanForward
#   endif
static unsigned int __inline TRAILING_ZEROS(size_t x)
{
	unsigned int msb;
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

// Fallback threshold
#define EXTERN_SIZE               (8 * 1024)

// The basic block with 8 BYTES.
#define BLK_BASE                  (8)

// The count of bytes managed by a byte, Using 1 bit to represent BLK_BASE bytes.
#define BMPBYTE_CBS               (BLK_BASE * 8)

// The count of bytes managed by a size_t.
#define BMPLONG_CBS               (BMPBYTE_CBS * sizeof(size_t))

// byte to bitmap index
#define BMPLONG_INDEX(p)          ((p) / BMPLONG_CBS)

// ceil(byte) to bitmap index
#define BMPLONG_INDEX_CEIL(p)     (BMPLONG_INDEX( (p) + (BMPLONG_CBS - BLK_BASE)))

// bit position in [0-31] or [0-63]
#define BMPLONG_BIT_POSITION(p)   (((p) % BMPLONG_CBS) / BLK_BASE)

#define SLAB_SIZE                 (64 * 1024)
#define BMPBYTE_SIZE              (SLAB_SIZE / BMPBYTE_CBS)
#define BMPLONG_SIZE              (BMPBYTE_SIZE / sizeof(size_t))
#define META_SIZE                 (BMPBYTE_SIZE - (SLAB_SIZE - BMPBYTE_SIZE) / BMPBYTE_CBS)

#if (META_SIZE < 16)
#   error TODO(BLK_BASE & SLAB_SIZE)
#endif

#define BPTR(p)                   ((unsigned char *)(p))

#define COMMIT_BASE               (8 * 1024)
#if defined(_MSC_VER)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   define VALLOC(size)            VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE)
#   define VCOMMIT(ptr, size)      VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE)
#   define VRESET(ptr, size)       VirtualAlloc(ptr, size, MEM_RESET, PAGE_NOACCESS)
#   define VFREE(ptr)              VirtualFree(ptr, 0, MEM_RELEASE)
#else
#include <sys/mman.h>
#   define VALLOC(size)            mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |MAP_ANONYMOUS, -1, 0)
#   define VCOMMIT(ptr, size)
#   define VRESET(ptr, size)
#   define VFREE(ptr)              munmap(ptr, SLAB_SIZE)
#endif

/*
 * Using 1 bit to represent 8 bytes(BLK_BASE)
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

// Only for a block with size >= (KFREELIST_MAX * BLK_BASE)
#define OWNERSLAB(m)      (*((void **)(m) + 1))
#define FREE_NEXT(m)      (*(void **)(m))
#define FREE_HEAD(k, i)   ((k)->freelist[i])

// memset(slab, 1024, 0). reduce the dependency on <stdlib.h>
static inline void slab_metazero(struct slab *slab)
{
	int i = 0;
	size_t *const bmp = slab->meta.bitmap;
	while (i < BMPLONG_SIZE) {
		bmp[i++] = 0;
		bmp[i++] = 0;
	}
}

static inline void slab_init(struct slab *slab)
{
	slab_committed(slab) = COMMIT_BASE;
	// starting point
	slab_pos(slab) = BMPBYTE_SIZE;
	//
	slab_next(slab) = NULL;
	// mark the first block for block_add
	slab->meta.bitmap[META_SIZE / sizeof(size_t)] = 1;
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
	VCOMMIT(slab, COMMIT_BASE);
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
	} else if (index < BMPLONG_INDEX_CEIL(slab_pos(slab))) {
		bits = slab->meta.bitmap[index++];
		begin = -1; // prev begin
	} else {
		return BLK_BASE;
	}
	if (bits)
		return (1 + TRAILING_ZEROS(bits)) * BLK_BASE;
	int cnt = BIT_CNT_MAX - begin;
	int max = BMPLONG_INDEX_CEIL(slab_pos(slab));
	while (index < max) {
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
	// Just simply mark the next point as a separator.
	int next = offset + size;
	slab_pos(slab) = next;
#if defined(_MSC_VER)
	int committed = slab_committed(slab);
	if (next >= committed) {
		if (SLAB_SIZE > committed) {
			int diff = next - committed;
			int extra = diff ? ALIGN_UP(diff, COMMIT_BASE) : COMMIT_BASE;
			VCOMMIT(BPTR(slab) + committed, extra);
			slab_committed(slab) = committed + extra;
		}
		if (next >= SLAB_SIZE)
			return next == SLAB_SIZE ? block : NULL;
	}
#else
	if (next >= SLAB_SIZE)
		return next == SLAB_SIZE ? block : NULL;
#endif
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

static void *free_pickup(struct kuai *kuai, int size)
{
	int i = free_index(size);
	void *block = FREE_HEAD(kuai, i);
	if (block && i) {
		FREE_HEAD(kuai, i) = FREE_NEXT(block);
		return block;
	}
	void *prev = NULL;
	// TODO : use PMAP instead of here.
	while (block) {
		struct slab *slab = OWNERSLAB(block);
		int real = block_size(slab, block);
		if (real < size) {
			prev  = block;
			block = FREE_NEXT(block);
			continue;
		}
		if (real >= size + (BLK_BASE * KFREELIST_MAX)) { // Do Splits
			void *next = BPTR(block) + size;
			block_split(slab, next);
			OWNERSLAB(next)  = slab;
			FREE_NEXT(next)  = FREE_NEXT(block);
			FREE_NEXT(block) = next;
		}
		if (prev) {
			FREE_NEXT(prev) = FREE_NEXT(block);
		} else {
			FREE_HEAD(kuai, i) = FREE_NEXT(block);
		}
		break;
	}
	return block;
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
	// exclude external allocating
	while (slab && !slab_isinner(slab)) {
		struct slab *next = slab_next(slab);
		free(slab);
		slab = next;
	}
	slab_head(kuai) = slab ? slab_next(slab) : NULL;
	kuai_destroy(kuai);
	if (!slab)
		return;

	if (slab_committed(slab) > COMMIT_BASE)
		VRESET(slab + COMMIT_BASE, slab_committed(slab) - COMMIT_BASE);

	slab_metazero(slab);
	slab_init(slab);
	slab_head(kuai) = slab;
}

void *kuai_alloc(struct kuai *kuai, int size)
{
	// if size > 8KB
	if (size > EXTERN_SIZE) {
		size += META_SIZE;
		struct slab *slab = malloc(size);
		if (!slab)
			return NULL;
		slab_pos(slab) = size;
		slab_isinner(slab) = 0;
		slab_next(slab) = NULL;
		// Add to tail
		struct slab **prev = (struct slab **)&slab_head(kuai);
		while (*prev)
			prev = &slab_next(*prev);
		*prev = slab;
		return BPTR(slab) + META_SIZE;
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
	struct slab *prev = NULL;
	struct slab *slab = slab_head(kuai);
	while (slab) {
		if (BPTR(block) > BPTR(slab) && BPTR(block) < BPTR(slab) + slab_pos(slab))
			break;
		prev = slab;
		slab = slab_next(slab);
	}
	if (!slab)
		return;
	// external alloc
	if (!slab_isinner(slab)) {
		if (prev)
			slab_next(prev) = slab_next(slab);
		free(slab);
		return;
	}
	if (!block_validate(slab, block))
		return;
	int size = block_size(slab, block);
	int i = free_index(size);
	FREE_NEXT(block) = FREE_HEAD(kuai, i);
	FREE_HEAD(kuai, i) = block;
	if (i == 0)
		OWNERSLAB(block) = slab;
}
