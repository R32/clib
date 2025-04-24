/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * most of this code is stolen from the hashlink/buffer.c by Haxe Foundation
 */

#include <float.h>
#include <stdio.h> // snprintf
#include "strbuf.h"

#define CHK_HEAD(buf) ((buf)->inner.head)
#define CHK_TAIL(buf) ((buf)->inner.tail)

void strbuf_init(struct strbuf *buf, int init)
{
	buf->inner = (struct buffer){ 0 };
	buffer_append_chunk(&buf->inner, init, sizeof(char));
}

static void strbuf_append_new(struct strbuf *buf, char *src, int len)
{
	struct chunk *chk = buffer_append_chunk(&buf->inner, len, sizeof(char));
	memcpy(chk->data, src, len);
	chk->pos = len;
}

void strbuf_append_char(struct strbuf *buf, char c)
{
	struct chunk *chk = CHK_TAIL(buf);
	if (!chk || chk->pos == chk->len)
		chk = buffer_append_chunk(&buf->inner, 0, sizeof(char));
	chk->data[chk->pos++] = c;
}

void strbuf_append_string(struct strbuf *buf, char *string, int len)
{
	if (!string)
		return;
	if (len < 0)
		len = strlen(string);
	struct chunk *chk = CHK_TAIL(buf);
	if (chk) {
		int rest = chk->len - chk->pos;
		if (rest >= len) {
			memcpy(chk->data + chk->pos, string, len);
			chk->pos += len;
			return;
		} else {
			memcpy(chk->data + chk->pos, string, rest);
			chk->pos += rest;
			string += rest;
			len -= rest;
		}
	}
	strbuf_append_new(buf, string, len);
}

void strbuf_append_int(struct strbuf *buf, int i)
{
	char array[16];
	int len = snprintf(array, 16, "%d", i);
	strbuf_append_string(buf, array, len);
}

static int trim_tail_zeros(char *ptr, int len)
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

#ifdef _MSC_VER
#   ifndef snprintf
#       define snprintf _snprintf
#   endif
#endif

void strbuf_append_float(struct strbuf *buf, float f, int fixed)
{
	char array[16];
	int len;
	if (fixed <= 0)
		fixed = 9;
	len = snprintf(array, 16, "%.*f", fixed, f);
	strbuf_append_string(buf, array, trim_tail_zeros(array, len));
}

void strbuf_append_double(struct strbuf *buf, double lf, int fixed)
{
	char array[32];
	int len;
	if (fixed <= 0)
		fixed = 17;
	len = snprintf(array, 32, "%.*g", fixed, lf + DBL_EPSILON);
	strbuf_append_string(buf, array, trim_tail_zeros(array, len));
}

/*
 * ```c
 * int len = strbuf_length(strbuf);
 * char *ptr = malloc(len + 1);
 * strbuf_to_string(strbuf, ptr);
 * ```
 */
void strbuf_to_string(struct strbuf *buf, char *out)
{
	char *dst = out;
	buffer_for_each(&buf->inner, chk) {
		memcpy(dst, chk->data, chk->pos);
		dst += chk->pos;
	}
	*dst = 0;
}
