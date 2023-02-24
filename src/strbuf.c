/*
 * SPDX-License-Identifier: GPL-2.0
 */

/*
 * most of this code is taken from the hashlink/buffer.c by Haxe Foundation
 */
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include "strbuf.h"

struct chunk {
	int pos;
	int len; // length(chunk->mem)
	union {
		struct chunk *next;
		double __x;
	};
	char data[0];
};

#define chk_data(chk)  ((chk)->data)
#define chk_head(buf)  ((buf)->chunks)
#define chk_next(chk)  ((chk)->next)

void strbuf_init(struct strbuf *buf)
{
	buf->csize = 128;
	buf->length = 0;
	buf->chunks = NULL;
}

void strbuf_reset(struct strbuf *buf)
{
	struct chunk *next;
	struct chunk *chk = chk_head(buf);
	if (!chk)
		return;
	next = chk_next(chk);
	// Keep the first chunk
	chk_next(chk) = NULL;
	chk->pos = 0;
	buf->length = 0;
	// Release the remaining chunks
	chk = next;
	while (chk) {
		next = chk_next(next);
		rb_free(chk);
		chk = next;
	}
}

void strbuf_release(struct strbuf *buf)
{
	struct chunk *next;
	struct chunk *chk = chk_head(buf);
	while (chk) {
		next = chk_next(chk);
		rb_free(chk);
		chk = next;
	}
	strbuf_init(buf);
}

static void strbuf_append_new(struct strbuf *buf, char *src, int len)
{
	while (buf->length >= (buf->csize << 2))
		buf->csize <<= 1;
	int size = len < buf->csize ? buf->csize : len;
	struct chunk *chk = rb_malloc(sizeof(struct chunk) + size);
	if (!chk) {
		// TODO
	}
	memcpy(chk_data(chk), src, len);
	chk->len = size;
	chk->pos = len;
	chk_next(chk) = chk_head(buf);
	chk_head(buf) = chk;
}

void strbuf_append_char(struct strbuf *buf, char c)
{
	struct chunk *chk = chk_head(buf);
	buf->length++;
	if (chk && chk->pos < chk->len) {
		chk->data[chk->pos++] = c;
		return;
	}
	strbuf_append_new(buf, &c, 1);
}

void strbuf_append_string(struct strbuf *buf, char *string, int len)
{
	if (!string)
		return;
	if (len < 0)
		len = strlen(string);
	buf->length += len;
	struct chunk *chk = chk_head(buf);
	if (chk) {
		int free = chk->len - chk->pos;
		if (free >= len) {
			memcpy(chk_data(chk) + chk->pos, string, len);
			chk->pos += len;
			return;
		} else {
			memcpy(chk_data(chk) + chk->pos, string, free);
			chk->pos += free;
			string += free;
			len -= free;
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

static int trim_tail_zero(char *ptr, int len)
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

void strbuf_append_float(struct strbuf *buf, float f, int fixed)
{
	char array[16];
	int len;
	if (fixed < 0) {
		len = snprintf(array, 16, "%f"  ,        f);
	} else {
		len = snprintf(array, 16, "%.*f", fixed, f);
	}
	strbuf_append_string(buf, array, trim_tail_zero(array, len));
}

void strbuf_append_double(struct strbuf *buf, double lf, int fixed)
{
	char array[32];
	int len;
	if (fixed < 0) {
		len = snprintf(array, 32, "%g"  ,        lf + DBL_EPSILON);
	} else {
		len = snprintf(array, 32, "%.*g", fixed, lf + DBL_EPSILON);
	}
	strbuf_append_string(buf, array, trim_tail_zero(array, len));
}

/*
 * example: 
 * ```c
 * int len = buf->length;
 * char *ptr = malloc(len + 1);
 * strbuf_to_string(buf, ptr);
 * ```
 */
void strbuf_to_string(struct strbuf *buf, char *out)
{
	char *ptr = out + buf->length;
	*ptr = 0;
	struct chunk *chk = chk_head(buf);
	while (chk) {
		ptr -= chk->pos;
		memcpy(ptr, chk_data(chk), chk->pos);
		chk = chk_next(chk);
	}
}

static int stream_write_rec(struct chunk *chk, FILE *stream)
{
	if (!chk)
		return 0;
	int size = stream_write_rec(chk_next(chk), stream);
	return size + fwrite(chk_data(chk), sizeof(char), chk->pos, stream);	
}

int strbuf_to_file(struct strbuf *buf, FILE *stream)
{
	struct chunk *chk = chk_head(buf);
	int size = stream_write_rec(chk, stream);
	fflush(stream);
	return size;
}
