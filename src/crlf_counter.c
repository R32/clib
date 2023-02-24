/*
 * SPDX-License-Identifier: GPL-2.0
 */
#include "crlf_counter.h"

struct chunk {
	int pos;
	int len; // elements length
	union {
		struct chunk *next;
		double __x;
	};
	int data[0];
};

#define chk_head(buf)  ((buf)->chunks)
#define chk_next(chk)  ((chk)->next)

void crlf_init(struct crlf_counter *crlf)
{
	strbuf_init((struct strbuf *)crlf);
}

void crlf_release(struct crlf_counter *crlf)
{
	strbuf_release((struct strbuf *)crlf);
}

static void crlf_append_new(struct crlf_counter *crlf, int pos)
{
	while (crlf->length >= (crlf->csize << 2))
		crlf->csize <<= 1;
	int size = crlf->csize;
	struct chunk *chk = rb_malloc(sizeof(struct chunk) + size * sizeof(int));
	if (!chk) {
		// TODO
	}
	chk->len = size;
	chk->pos = 0;
	chk_next(chk) = chk_head(crlf);
	chk_head(crlf) = chk;

	if (crlf->length == 1) {
		chk->data[chk->pos++] = 0; // (line 1, column 1) at 0;
		crlf->length++;
	}
	// chk.push(pos)
	chk->data[chk->pos++] = pos;
}

void crlf_add(struct crlf_counter *crlf, int pos)
{
	struct chunk *chk = chk_head(crlf);
	crlf->length++;
	if (chk && chk->pos < chk->len) {
		chk->data[chk->pos++] = pos;
		return;
	}
	crlf_append_new(crlf, pos);
}

static int crlf_index2addr(struct chunk *chk, int index, int **addr)
{
	if (!chk)
		return index;
	index = crlf_index2addr(chk_next(chk), index, addr);
	if (index >= 0) {
		if (index >= chk->len)
			return index - chk->len;
		*addr = &chk->data[index];
	}
	return -1;
}

static int crlf_pos(struct crlf_counter *crlf, int index) {
	int *pos = NULL;
	crlf_index2addr(chk_head(crlf), index, &pos);
	return pos ? *pos : 0;
}

struct lncolumn crlf_get(struct crlf_counter *crlf, int pos)
{
	// bsearch
	int i = 0;
	int j = crlf->length - 1;
	while (i <= j) {
		int k = (i + j) >> 1;
		int p = crlf_pos(crlf, k);
		if (pos < p) {
			j = k - 1;
		} else {
			i = k + 1;
			if (k < j && pos >= crlf_pos(crlf, i)) {
				continue;
			}
			return (struct lncolumn){.line = i, .column = pos - p + 1};
		}
	}
	return (struct lncolumn){.line = 1, .column = pos + 1};
}
