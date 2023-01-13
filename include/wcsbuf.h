/*
* SPDX-License-Identifier: GPL-2.0
*/
#ifndef R_WCSBUF_H
#define R_WCSBUF_H
#include "rclibs.h"

#ifndef rb_malloc
#	define rb_malloc malloc
#endif
#ifndef rb_free
#	define rb_free free
#endif

struct wcsbuf {
	int csize;
	int length; // length of input;
	void *buffer;
};

#define wcsbuf_length(buf) ((buf)->length)

C_FUNCTION_BEGIN

void wcsbuf_init(struct wcsbuf *buf);
void wcsbuf_release(struct wcsbuf *buf);

void wcsbuf_append_char(struct wcsbuf *buf, wchar_t c);
void wcsbuf_append_string(struct wcsbuf *buf, wchar_t *string, int len);
void wcsbuf_append_int(struct wcsbuf *buf, int i);
void wcsbuf_append_float(struct wcsbuf *buf, float f, int fixed);
void wcsbuf_append_double(struct wcsbuf *buf, double lf, int fixed);

void wcsbuf_to_string(struct wcsbuf *buf, wchar_t *out);
int  wcsbuf_to_file(struct wcsbuf *buf, FILE *stream);

// write wcsbuf to file with UTF8
int  wcsbuf_to_file_utf8(struct wcsbuf *buf, FILE *stream);
C_FUNCTION_END
#endif
