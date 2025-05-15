/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */
#ifndef LWM_UCSBUF_H
#define LWM_UCSBUF_H

#include "buffer.h"
#include "ucs2.h"

struct ucsbuf {
	struct buffer inner;
};

#define ucsbuf_length(buf)   buffer_length(&(buf)->inner)
#define ucsbuf_reset(buf)    buffer_reset(&(buf)->inner)
#define ucsbuf_release(buf)  buffer_release(&(buf)->inner)


void ucsbuf_init(struct ucsbuf *buf, int init);

void ucsbuf_append_char(struct ucsbuf *buf, uchar c);
void ucsbuf_append_string(struct ucsbuf *buf, uchar *string, int len);
void ucsbuf_append_int(struct ucsbuf *buf, int i);
void ucsbuf_append_float(struct ucsbuf *buf, float f, int fixed);
void ucsbuf_append_double(struct ucsbuf *buf, double lf, int fixed);

void ucsbuf_to_string(struct ucsbuf *buf, uchar *out);

#endif
