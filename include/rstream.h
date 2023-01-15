/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_STREAM_H
#define R_STREAM_H
#include "rlex.h"

struct rstream_tok {
	struct rlex_position pos;
	int state;
	int term;
	union {
		struct {
			int   i32;
			int   high;
		};
		void      *value;
		long long i64;
		float     f32;
		double    f64;
	};
};

struct rstream {
	int head;
	int tail;
	struct rlex *lex;
	struct rstream_tok cached[64];
};

#ifdef __cplusplus
extern "C" {
#endif


void rstream_junk(struct rstream *stream, int n);

void rstream_init(struct rstream *stream, struct rlex *lex);

void rstream_unshift(struct rstream *stream, struct rstream_tok *src);

/*
 *IMPORTANT:
 *
 * The returned (rstream_tok *) will point to an element from the array(stream.cached[N]), 
 *
 * Its internal value will be changed at any time.
 */

struct rstream_tok *rstream_peek(struct rstream *stream, int i);

struct rstream_tok *rstream_next(struct rstream *stream);

struct rstream_tok *rstream_reduce(struct rstream *stream, int width);


#ifdef __cplusplus
}
#endif

#endif
