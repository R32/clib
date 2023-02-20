/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_CRLF_COUNTER_H
#define R_CRLF_COUNTER_H
#include "strbuf.h"

struct lncolumn {
	int line;
	int column; // start at 1
};

/*
 * This module is often used with lexer to save the position of '\n'
 */
struct crlf_counter {
	int csize;
	int length;
	void *chunks;
};

C_FUNCTION_BEGIN

void crlf_init(struct crlf_counter *crlf);
void crlf_release(struct crlf_counter *crlf);

void crlf_add(struct crlf_counter *crlf, int pos);
struct lncolumn crlf_get(struct crlf_counter *crlf, int pos);

C_FUNCTION_END
#endif
