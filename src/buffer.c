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

	chunk_release(CHK_NEXT(keep));

	keep->pos = 0;
	CHK_NEXT(keep) = NULL;
	CHK_HEAD(buff) = keep;
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
