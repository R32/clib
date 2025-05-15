/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#include "mempool.h"

#define FREE_HEAD(pool)  ((pool)->freehead)
#define FREE_NEXT(ptr)   (*(void **)(ptr))

void mempool_init(struct mempool *pool, int size, int init)
{
	*pool = (struct mempool){ .size = size };
	buffer_append_chunk(&pool->inner, init, size);
}

void *mempool_alloc(struct mempool *pool)
{
	void *free = FREE_HEAD(pool);
	if (free == NULL) {
		return buffer_incr(&pool->inner, pool->size);
	}
	FREE_HEAD(pool) = FREE_NEXT(free);
	return free;
}

void mempool_free(struct mempool *pool, void *ptr)
{
	FREE_NEXT(ptr) = FREE_HEAD(pool);
	FREE_HEAD(pool) = ptr;
}
