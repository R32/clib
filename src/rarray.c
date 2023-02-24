/*
 * SPDX-License-Identifier: GPL-2.0
 */

#include "rarray.h"

struct rarray_head {
	int len;
	int cap;
	char data[0];
};

#define hd_to_base(head)     ((prarray_base)(head)->data)
#define hd_from_base(base)   (container_of(((void *)(base)), struct rarray_head, data))

static void phead_realloc(struct rarray *prar, int cap, int len)
{
	struct rarray_head *head = prar->base ? hd_from_base(prar->base) : NULL;
	head = ra_realloc(head, sizeof(struct rarray_head) + prar->size * cap);
	head->cap = cap;
	head->len = len;
	prar->base = hd_to_base(head);
}

void rarray_init(struct rarray *prar, int elemsize)
{
	prar->size = elemsize;
	prar->base = NULL;
}

void rarray_release(struct rarray *prar)
{
	if (!prar->base)
		return;
	struct rarray_head *head = hd_from_base(prar->base);
	ra_free(head);
	prar->base = NULL;
}

void rarray_grow(struct rarray *prar, int cap)
{
	cap = cap < 16 ? 16 : ALIGN_POW2(cap, 8);
	if (!prar->base) {
		phead_realloc(prar, cap, 0);
		return;
	}
	int len = hd_from_base(prar->base)->len;
	phead_realloc(prar, cap, len > cap ? cap : len);
}

int rarray_len(struct rarray *prar)
{
	if (!prar->base)
		return 0;
	return hd_from_base(prar->base)->len;
}

void rarray_setlen(struct rarray *prar, int len)
{
	if (prar->base) {
		struct rarray_head *head = hd_from_base(prar->base);
		if (len <= head->cap) {
			head->len = len;
			return;
		}
	}
	int cap = len < 16 ? 16 : ALIGN_POW2(len, 8);
	phead_realloc(prar, cap, len);
}

int rarray_cap(struct rarray *prar)
{
	if (!prar->base)
		return 0;
	return hd_from_base(prar->base)->cap;
}

int rarray_push(struct rarray *prar, void *value)
{
	if (!prar->base)
		phead_realloc(prar, 16, 0);
	struct rarray_head *head = hd_from_base(prar->base);
	if (head->len == head->cap) {
		phead_realloc(prar, head->cap * 2, head->len);
		head = hd_from_base(prar->base);
	}
	memcpy(head->data + (head->len++ * prar->size), value, prar->size);
	return head->len;
}

void *rarray_pop(struct rarray *prar)
{
	if (!prar->base)
		return NULL;
	struct rarray_head *head = hd_from_base(prar->base);
	if (head->len > 0)
		return head->data + (--head->len * prar->size);
	return NULL;
}

void *rarray_get(struct rarray *prar, int index)
{
	if (!prar->base)
		return NULL;
	struct rarray_head *head = hd_from_base(prar->base);
	if (index >= 0 && index < head->len)
		return head->data + (index * prar->size);
	return NULL;
}

void rarray_set(struct rarray *prar, int index, void *value)
{
	if (index < 0)
		return;
	int len = prar->base ? hd_from_base(prar->base)->len : 0;
	if (len <= index) {
		len = index + 1;
		int cap = len < 16 ? 16 : ALIGN_POW2(len, 8);
		phead_realloc(prar, cap, len);
	}
	struct rarray_head *head = hd_from_base(prar->base);
	memcpy(head->data + (index * prar->size), value, prar->size);
}
