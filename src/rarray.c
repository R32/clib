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
#define hd_from_base(base)   (container_of(base, struct rarray_head, data))

static struct rarray_head *rarray_head_new(struct rarray_head *head, int elemsize, int cap)
{
	cap = cap < 16 ? 16 : ALIGN_POW2(cap, 8);
	elemsize = elemsize < 8 ? 8 : ALIGN_POW2(elemsize, 8);
	head = ra_realloc(head, sizeof(struct rarray_head) + elemsize * cap);
	head->cap = cap;
	return head;
}

void rarray_new(struct rarray *prar, int elemsize, int cap)
{
	struct rarray_head *head = rarray_head_new(NULL, elemsize, cap);
	head->len = 0;
	prar->base = hd_to_base(head);
}

void rarray_free(struct rarray *prar)
{
	struct rarray_head *head = hd_from_base(prar->base);
	ra_free(head);
	prar->base = NULL;
}

void rarray_grow(struct rarray *prar, int elemsize, int cap)
{
	struct rarray_head *head = prar->base ? hd_from_base(prar->base) : NULL;
	head = rarray_head_new(head, elemsize, cap);
	if (head->len > head->cap)
		head->len = head->cap;
	prar->base = hd_to_base(head);
}

int rarray_len(struct rarray *prar)
{
	return hd_from_base(prar->base)->len;
}

int rarray_cap(struct rarray *prar)
{
	return hd_from_base(prar->base)->cap;
}

int rarray_push(struct rarray *prar, int elemsize, void *value)
{
	struct rarray_head *head = hd_from_base(prar->base);
	if (head->len == head->cap) {
		rarray_grow(prar, elemsize, head->cap * 2);
		head = hd_from_base(prar->base);
	}
	memcpy(head->data + (head->len++ * elemsize), value, elemsize);
	return head->len;
}

void *rarray_pop(struct rarray *prar, int elemsize)
{
	struct rarray_head *head = hd_from_base(prar->base);
	if (head->len > 0)
		return head->data + (--head->len * elemsize);
	return NULL;
}

void *rarray_get(struct rarray *prar, int elemsize, int index)
{
	struct rarray_head *head = hd_from_base(prar->base);
	if (index >= 0 && index < head->len)
		return head->data + (index * elemsize);
	return NULL;
}

void rarray_set(struct rarray *prar, int elemsize, int index, void *value)
{
	struct rarray_head *head = hd_from_base(prar->base);
	if (index >= 0 && index < head->len)
		memcpy(head->data + (index * elemsize), value, elemsize);
}
