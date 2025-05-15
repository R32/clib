/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#include <float.h>
#include <string.h> // memcpy
#include "ucs2.h"
#include "ucsbuf.h"

#define CHK_HEAD(buf) ((buf)->inner.head)
#define CHK_TAIL(buf) ((buf)->inner.tail)
#define CHK_DATA(chk) ((uchar *)(chk)->data)

void ucsbuf_init(struct ucsbuf *buf, int init)
{
	buf->inner = (struct buffer){ 0 };
	buffer_append_chunk(&buf->inner, init, sizeof(uchar));
}

static void ucsbuf_append_new(struct ucsbuf *buf, uchar *src, int len)
{
	struct chunk *chk = buffer_append_chunk(&buf->inner, len, sizeof(uchar));
	memcpy(chk->data, src, len * sizeof(uchar));
	chk->pos = len;
}

void ucsbuf_append_char(struct ucsbuf *buf, uchar c)
{
	struct chunk *chk = CHK_TAIL(buf);
	if (!chk || chk->pos == chk->len)
		chk = buffer_append_chunk(&buf->inner, 0, sizeof(uchar));
	CHK_DATA(chk)[chk->pos++] = c;
}


void ucsbuf_append_string(struct ucsbuf *buf, uchar *string, int len)
{
	if (!string)
		return;
	if (len < 0)
		len = wcslen(string);
	struct chunk *chk = CHK_TAIL(buf);
	if (chk) {
		int rest = chk->len - chk->pos;
		if (rest >= len) {
			memcpy(CHK_DATA(chk) + chk->pos, string, len * sizeof(uchar));
			chk->pos += len;
			return;
		} else {
			memcpy(CHK_DATA(chk) + chk->pos, string, rest * sizeof(uchar));
			chk->pos += rest;
			string += rest;
			len -= rest;
		}
	}
	ucsbuf_append_new(buf, string, len);
}

void ucsbuf_append_int(struct ucsbuf *buf, int i)
{
	uchar array[12];
	int len = itoua(i, array);
	ucsbuf_append_string(buf, array, len);
}

void ucsbuf_append_float(struct ucsbuf *buf, float f, int precision)
{
	uchar array[16];
	int len = ftoua(f, array, precision, 0);
	ucsbuf_append_string(buf, array, len);
}

void ucsbuf_append_double(struct ucsbuf *buf, double g, int precision)
{
	uchar array[32];
	int len = dtoua(g, array, precision, 0);
	ucsbuf_append_string(buf, array, len);
}

/*
 * ```c
 * int len = wcsbuf_length(buf);
 * uchar *wcs = malloc((len + 1) * uchar);
 * wcsbuf_to_string(buf, wcs);
 * ```
 */
void ucsbuf_to_string(struct ucsbuf *buf, uchar *out)
{
	uchar *dst = out;
	buffer_for_each(&buf->inner, chk) {
		memcpy(dst, CHK_DATA(chk), chk->pos * sizeof(uchar));
		dst += chk->pos;
	}
	*dst = 0;
}
