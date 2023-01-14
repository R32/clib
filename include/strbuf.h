/*
* SPDX-License-Identifier: GPL-2.0
*/
#ifndef R_STRBUF_H
#define R_STRBUF_H
#include "rclibs.h"

#ifndef rb_malloc
#	define rb_malloc malloc
#endif
#ifndef rb_free
#	define rb_free free
#endif

struct strbuf {
	int csize;
	int length; // length of input;
	void *buffer;
};

#define strbuf_length(buf) ((buf)->length)

C_FUNCTION_BEGIN

void strbuf_init(struct strbuf *buf);
void strbuf_reset(struct strbuf *buf);
void strbuf_release(struct strbuf *buf);

void strbuf_append_char(struct strbuf *buf, char c);
void strbuf_append_string(struct strbuf *buf, char *string, int len);
void strbuf_append_int(struct strbuf *buf, int i);
void strbuf_append_float(struct strbuf *buf, float f, int fixed);
void strbuf_append_double(struct strbuf *buf, double lf, int fixed);

void strbuf_to_string(struct strbuf *buf, char *out);
int  strbuf_to_file(struct strbuf *buf, FILE *stream);

C_FUNCTION_END
#endif
