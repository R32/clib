/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#ifndef LWM_CRLF_COUNTER_H
#define LWM_CRLF_COUNTER_H
#include "buffer.h"

struct line_column {
	int line;
	int column; // start at 1
};

/*
 * This module typically works with lexer/parser to record the next position of '\n'.
 */
struct crlf_counter {
	struct buffer inner;
	// TODO : filename
};


#define crlf_length(crlf) buffer_length(&(crlf)->inner)
#define crlf_reset(crlf)  do {        \
	buffer_reset(&(crlf)->inner); \
	(crlf)->inner.head->pos++;    \
} while(0)

#define crlf_release(crlf) buffer_release(&(crlf)->inner)

void crlf_init(struct crlf_counter *crlf, int init);
void crlf_add(struct crlf_counter *crlf, int pos);
struct line_column crlf_search(struct crlf_counter *crlf, int pos);

#endif
