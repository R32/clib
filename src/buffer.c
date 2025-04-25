/*
 * SPDX-License-Identifier: GPL-2.0
 */

#include "buffer.h"

#define CHK_HEAD(buf)    ((buf)->head)
#define CHK_TAIL(buf)    ((buf)->tail)

static void inline chunk_release(struct chunk *chk)
{
	struct chunk *next;
	while (chk) {
		next = CHK_NEXT(chk);
		CHK_FREE(chk);
		chk = next;
	}
}

void buffer_release(struct buffer *buff)
{
	chunk_release(CHK_HEAD(buff));
	*buff = (struct buffer){0};
}

/*
 * Just keep the first chunk
 */
void buffer_reset(struct buffer *buff)
{
	struct chunk *keep = CHK_HEAD(buff);
	if (!keep)
		return;
	// Release all chunks except first
	chunk_release(CHK_NEXT(keep));

	keep->pos = 0;
	CHK_NEXT(keep) = NULL;
	CHK_TAIL(buff) = keep;
}

int buffer_length(struct buffer *buff)
{
	int acc = 0;
	buffer_for_each(buff, chk)
		acc += chk->pos;
	return acc;
}

struct chunk *buffer_append_chunk(struct buffer *buff, int len, int size)
{
	struct chunk *tail = CHK_TAIL(buff);
	if (tail) {
		int count = 0;
		buffer_for_each(buff, chk)
			count++;
		// grows : 32, 32, 64, 64, 128 ....
		int grows = count & 1 ? tail->len : tail->len * 2;
		if (len < grows)
			len = grows;
	} else if (len < 32) {
		len = 32;
	}
	struct chunk *chk = CHK_ALLOC(sizeof(struct chunk) + len * (size & 0xFFFF));
	if (!chk) {
		FATAL_OUT_OF_MEMORY(chk);
		return NULL;
	}
	chk->pos = 0;
	chk->len = len;

	CHK_NEXT(chk) = NULL;
	if (tail)
		CHK_NEXT(tail) = chk;
	else
		CHK_HEAD(buff) = chk;
	CHK_TAIL(buff) = chk;
	return chk;
}


void *buffer_incr(struct buffer *buff, int size)
{
	struct chunk *chk = CHK_TAIL(buff);
	if (!chk || chk->pos == chk->len)
		chk = buffer_append_chunk(buff, 0, size);
	return &chk->data[size * chk->pos++];
}

void *buffer_index(struct buffer *buff, int size, int index)
{
	buffer_for_each(buff, chk) {
		if (chk->pos > index)
			return &chk->data[index * size];
		index -= chk->pos;
	}
	return NULL;
}

void *buffer_bsearch(struct buffer *buff, int size, void *value, int (*compare)(const void*, const void*))
{
	int i = 0;
	int j = buffer_length(buff) - 1;
	while (i <= j) {
		int k = (i + j) >> 1;
		void *pt = buffer_index(buff, size, k);
		int sign = compare(value, pt);
		if (sign < 0) {
			j = k - 1;
		} else if (sign > 0) {
			i = k + 1;
		} else {
			return pt;
		}
	}
	return NULL;
}
