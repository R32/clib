/*
 * SPDX-License-Identifier: GPL-2.0
 */
#include <stdio.h> // swprintf in msvc
#include <float.h>
#include "wcsbuf.h"

#define CHK_HEAD(buf) ((buf)->inner.head)
#define CHK_TAIL(buf) ((buf)->inner.tail)
#define CHK_DATA(chk) ((wchar_t *)(chk)->data)

void wcsbuf_init(struct wcsbuf *buf, int init)
{
	buf->inner = (struct buffer){ 0 };
	buffer_append_chunk(&buf->inner, init, sizeof(wchar_t));
}

static void wcsbuf_append_new(struct wcsbuf *buf, wchar_t *src, int len)
{
	struct chunk *chk = buffer_append_chunk(&buf->inner, len, sizeof(wchar_t));
	memcpy(chk->data, src, len * sizeof(wchar_t));
	chk->pos = len;
}

void wcsbuf_append_char(struct wcsbuf *buf, wchar_t c)
{
	struct chunk *chk = CHK_TAIL(buf);
	if (!chk || chk->pos == chk->len)
		chk = buffer_append_chunk(&buf->inner, 0, sizeof(wchar_t));
	CHK_DATA(chk)[chk->pos++] = c;
}


void wcsbuf_append_string(struct wcsbuf *buf, wchar_t *string, int len)
{
	if (!string)
		return;
	if (len < 0)
		len = wcslen(string);
	struct chunk *chk = CHK_TAIL(buf);
	if (chk) {
		int rest = chk->len - chk->pos;
		if (rest >= len) {
			memcpy(CHK_DATA(chk) + chk->pos, string, len * sizeof(wchar_t));
			chk->pos += len;
			return;
		} else {
			memcpy(CHK_DATA(chk) + chk->pos, string, rest * sizeof(wchar_t));
			chk->pos += rest;
			string += rest;
			len -= rest;
		}
	}
	wcsbuf_append_new(buf, string, len);
}

void wcsbuf_append_int(struct wcsbuf *buf, int i)
{
	wchar_t array[16];
	int len = swprintf(array, 16, L"%d", i);
	wcsbuf_append_string(buf, array, len);
}

static int trim_tail_zero(wchar_t *ptr, int len)
{
	int i = 0;
	while (i < len && ptr[i++] != '.') {
	}
	i += 2; // Keep at least 2 zeros
	int count = 0;
	while (i < len) {
		if (ptr[i++] != '0') {
			count = 0;
			continue;
		}
		count++;
		if (i == len || count == 3)
			return i - count;
	}
	return len;
}

void wcsbuf_append_float(struct wcsbuf *buf, float f, int fixed)
{
	wchar_t array[16];
	int len;
	if (fixed <= 0) {
		len = swprintf(array, 16, L"%f"  ,        f);
	} else {
		len = swprintf(array, 16, L"%.*f", fixed, f);
	}
	wcsbuf_append_string(buf, array, trim_tail_zero(array, len));
}

void wcsbuf_append_double(struct wcsbuf *buf, double lf, int fixed)
{
	wchar_t array[32];
	int len;
	if (fixed <= 0) {
		len = swprintf(array, 32, L"%g"  ,        lf + DBL_EPSILON);
	} else {
		len = swprintf(array, 32, L"%.*g", fixed, lf + DBL_EPSILON);
	}
	wcsbuf_append_string(buf, array, trim_tail_zero(array, len));
}

/*
 * ```c
 * int len = wcsbuf_length(buf);
 * wchar_t *wcs = malloc((len + 1) * wchar_t);
 * wcsbuf_to_string(buf, wcs);
 * ```
 */
void wcsbuf_to_string(struct wcsbuf *buf, wchar_t *out)
{
	wchar_t *dst = out;
	buffer_for_each(&buf->inner, chk) {
		memcpy(dst, CHK_DATA(chk), chk->pos * sizeof(wchar_t));
		dst += chk->pos;
	}
	*dst = 0;
}
