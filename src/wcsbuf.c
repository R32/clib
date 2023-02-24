/*
 * SPDX-License-Identifier: GPL-2.0
 */

/*
 * most of this code is taken from the hashlink/buffer.c by Haxe Foundation
 */
#include "wcsbuf.h"

struct chunk {
	int pos;
	int len; // length(chunk->mem)
	union {
		struct chunk *next;
		double __x;
	};
	wchar_t data[0];
};

#define chk_data(chk)  ((chk)->data)
#define chk_head(buf)  ((buf)->chunks)
#define chk_next(chk)  ((chk)->next)

void wcsbuf_init(struct wcsbuf *buf)
{
	strbuf_init((struct strbuf *)buf);
}

void wcsbuf_reset(struct wcsbuf *buf)
{
	strbuf_reset((struct strbuf *)buf);
}

void wcsbuf_release(struct wcsbuf *buf)
{
	strbuf_release((struct strbuf *)buf);
}

static void wcsbuf_append_new(struct wcsbuf *buf, wchar_t *src, int len)
{
	while (buf->length >= (buf->csize << 2))
		buf->csize <<= 1;
	int size = len < buf->csize ? buf->csize : len;
	struct chunk *chk = rb_malloc(sizeof(struct chunk) + size * sizeof(wchar_t));
	if (!chk) {
		// TODO
	}
	memcpy(chk_data(chk), src, len * sizeof(wchar_t));
	chk->len = size;
	chk->pos = len;
	chk_next(chk) = chk_head(buf);
	chk_head(buf) = chk;
}

void wcsbuf_append_char(struct wcsbuf *buf, wchar_t c)
{
	struct chunk *chk = chk_head(buf);
	buf->length++;
	if (chk && chk->pos < chk->len) {
		chk->data[chk->pos++] = c;
		return;
	}
	wcsbuf_append_new(buf, &c, 1);
}

void wcsbuf_append_string(struct wcsbuf *buf, wchar_t *string, int len)
{
	if (!string)
		return;
	if (len < 0)
		len = wcslen(string);
	buf->length += len;
	struct chunk *chk = chk_head(buf);
	if (chk) {
		int free = chk->len - chk->pos;
		if (free >= len) {
			memcpy(chk_data(chk) + chk->pos, string, len * sizeof(wchar_t));
			chk->pos += len;
			return;
		} else {
			memcpy(chk_data(chk) + chk->pos, string, free * sizeof(wchar_t));
			chk->pos += free;
			string += free;
			len -= free;
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
	if (fixed < 0) {
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
	if (fixed < 0) {
		len = swprintf(array, 32, L"%g"  ,        lf + DBL_EPSILON);
	} else {
		len = swprintf(array, 32, L"%.*g", fixed, lf + DBL_EPSILON);
	}
	wcsbuf_append_string(buf, array, trim_tail_zero(array, len));
}

/*
 * example: 
 * ```c
 * wchar_t *wcs = malloc((buf->length + 1) * wchar_t);
 * wcsbuf_to_string(buf, wcs);
 * ```
 */
void wcsbuf_to_string(struct wcsbuf *buf, wchar_t *out)
{
	wchar_t *ptr = out + buf->length;
	*ptr = 0;
	struct chunk *chk = chk_head(buf);
	while (chk) {
		ptr -= chk->pos;
		memcpy(ptr, chk_data(chk), chk->pos * sizeof(wchar_t));
		chk = chk_next(chk);
	}
}

static int stream_write_rec(struct chunk *chk, FILE *stream)
{
	if (!chk)
		return 0;
	int size = stream_write_rec(chk_next(chk), stream);
	return size + fwrite(chk_data(chk), sizeof(wchar_t), chk->pos, stream);
}

int wcsbuf_to_file(struct wcsbuf *buf, FILE *stream)
{
	struct chunk *chk = chk_head(buf);
	int size = stream_write_rec(chk, stream);
	fflush(stream);
	return size;
}

static int stream_write_to_utf8(struct chunk *chk, FILE *stream)
{
	if (!chk)
		return 0;
	int size = stream_write_to_utf8(chk_next(chk), stream);
	int i = 0, k;
	while (i < chk->pos) {
		wchar_t c = chk_data(chk)[i++];
		if (c < 0x80) {
			fputc(c, stream);
		} else if (c < 0x800) {
			fputc(0xC0 | (c >> 6), stream);
			fputc(0x80 | (c & 63), stream);
		} else if (c >= 0xD800 && c <= 0xDFFF) {
			if (i == chk->pos)
				break;
			wchar_t c2 = chk_data(chk)[i++];
			k = (((c - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
			fputc(0xF0 | (k >> 18), stream);
			fputc(0x80 | ((k >> 12) & 63), stream);
			fputc(0x80 | ((k >> 6) & 63), stream);
			fputc(0x80 | (k & 63), stream);
		} else {
			fputc(0xE0 | (c >> 12), stream);
			fputc(0x80 | ((c >> 6) & 63), stream);
			fputc(0x80 | (c & 63), stream);
		}
	}
	return size + i;
}

// The total number of wchar_t character successfully written is returned
int wcsbuf_to_file_utf8(struct wcsbuf *buf, FILE *stream)
{
	struct chunk *chk = chk_head(buf);
	int size = stream_write_to_utf8(chk, stream);
	fflush(stream);
	return size;
}
