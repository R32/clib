/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#ifndef LWM_MEM_POOL_H
#define LWM_MEM_POOL_H

#include "buffer.h"

struct mempool {
	struct buffer inner;
	void *freehead;
	int size; // sizeof(block)
};

#define mempool_reset(pool) do{\
		buffer_reset(&(pool)->inner); \
		(pool)->freehead = NULL; \
	} while(0)

#define mempool_release(pool) do{\
		buffer_release(&(pool)->inner); \
		(pool)->freehead = NULL; \
	} while(0)

// size must be >= 8 bytes
void mempool_init(struct mempool *pool, int size, int init);
void *mempool_alloc(struct mempool *pool);
void mempool_free(struct mempool *pool, void *ptr);

#endif