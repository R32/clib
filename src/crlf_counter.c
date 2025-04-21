/*
 * SPDX-License-Identifier: GPL-2.0
 */

#include "crlf_counter.h"

#define CHK_HEAD(crlf) ((crlf)->inner.head)
#define CHK_TAIL(crlf) ((crlf)->inner.tail)
#define CHK_DATA(chk)  ((int *)(chk)->data)

void crlf_init(struct crlf_counter *crlf, int init)
{
	crlf->inner = (struct buffer){0};
	struct chunk *chk = buffer_append_chunk(&crlf->inner, init, sizeof(int));
	CHK_DATA(chk)[chk->pos++] = 0; // (line 1, column 1) at 0
}

void crlf_add(struct crlf_counter *crlf, int pos)
{
	struct chunk *chk = CHK_TAIL(crlf);
	if (!chk || chk->pos == chk->len) {
		chk = buffer_append_chunk(&crlf->inner, 64, sizeof(int));
		if (chk == CHK_HEAD(crlf))
			CHK_DATA(chk)[chk->pos++] = 0; // init
	}
	CHK_DATA(chk)[chk->pos++] = pos;
}

static int crlf_pos(struct crlf_counter *crlf, int index)
{
	int *pos = NULL;
	buffer_for_each(&crlf->inner, chk) {
		if (chk->pos > index)
			return CHK_DATA(chk)[index];
		index -= chk->pos;
	}
	return 0;
}

struct line_column crlf_search(struct crlf_counter *crlf, int pos)
{
	// bsearch
	int i = 0;
	int j = buffer_length(&crlf->inner) - 1;
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
			return (struct line_column){.line = i, .column = pos - p + 1};
		}
	}
	return (struct line_column){.line = 1, .column = pos + 1};
}
