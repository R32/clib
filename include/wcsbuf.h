/*
 * SPDX-License-Identifier: GPL-2.0
 */
#ifndef LWM_WCSBUF_H
#define LWM_WCSBUF_H

#include <wchar.h>
#include "buffer.h"

/*
 * TODO : convert wchar_t to char16_t(<uchar.h>) for non-msvc
 */
struct wcsbuf {
	struct buffer inner;
};

#define wcsbuf_length(buf)   buffer_length(&(buf)->inner)
#define wcsbuf_reset(buf)    buffer_reset(&(buf)->inner)
#define wcsbuf_release(buf)  buffer_release(&(buf)->inner)


void wcsbuf_init(struct wcsbuf *buf, int init);

void wcsbuf_append_char(struct wcsbuf *buf, wchar_t c);
void wcsbuf_append_string(struct wcsbuf *buf, wchar_t *string, int len);
void wcsbuf_append_int(struct wcsbuf *buf, int i);
void wcsbuf_append_float(struct wcsbuf *buf, float f, int fixed);
void wcsbuf_append_double(struct wcsbuf *buf, double lf, int fixed);

void wcsbuf_to_string(struct wcsbuf *buf, wchar_t *out);

#endif
