/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 *
 * most of this code is stolen from the hashlink/buffer.c by Haxe Foundation
 */

#ifndef LWM_STRBUF_H
#define LWM_STRBUF_H

#include "buffer.h"

struct strbuf {
	struct buffer inner;
};


#define strbuf_length(buf)   buffer_length(&(buf)->inner)
#define strbuf_reset(buf)    buffer_reset(&(buf)->inner)
#define strbuf_release(buf)  buffer_release(&(buf)->inner)

void strbuf_init(struct strbuf *buf, int init);
void strbuf_append_char(struct strbuf *buf, char c);
void strbuf_append_string(struct strbuf *buf, char *string, int len);
void strbuf_append_int(struct strbuf *buf, int i);
void strbuf_append_float(struct strbuf *buf, float f, int fixed);
void strbuf_append_double(struct strbuf *buf, double lf, int fixed);
void strbuf_to_string(struct strbuf *buf, char *out);

#endif
