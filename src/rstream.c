/*
 * SPDX-License-Identifier: GPL-2.0
 */

#include "rstream.h"

#define RS_OFFSET(rs, i)   (&(rs)->cached[(rs)->head + (i)])
#define RS_LENGTH(rs)      ((rs)->tail - (rs)->head)
#define RL_TOKEN(rs)       ((rs)->lex->token((rs)->lex))

/*
 * NOTE: It can only be used manually, because there is no corresponding Parser Tool
 *
 * And I haven't tested it
 */

// peek(0) means peek current token
struct rstream_tok *rstream_peek(struct rstream *stream, int i)
{
	while (RS_LENGTH(stream) <= i) {
		int term = RL_TOKEN(stream);
		struct rstream_tok *tok = &stream->cached[stream->tail++];
		tok->term = term;
		tok->pos = stream->lex->pos;
	}
	return RS_OFFSET(stream, i);
}

void rstream_junk(struct rstream *stream, int n)
{
	if (n <= 0)
		return;
	int len = RS_LENGTH(stream);
	if (len > n) {
		stream->tail -= n;
		len -= n;
		int i = 0;
		struct rstream_tok *const cur = RS_OFFSET(stream, 0);
		while (i < len) {
			cur[i] = cur[i + n]; // struct copy from LEFT to RIGHT
			++i;
		}
	} else {
		stream->tail = stream->head;
	/*
		n -= len;
		while (n--) {
			RL_TOKEN(stream); // discad
		}
	*/
	}
}

void rstream_init(struct rstream *stream, struct rlex *lex)
{
	stream->lex = lex;
	stream->head = 0;
	stream->tail = 0;
}

struct rstream_tok *rstream_next(struct rstream *stream)
{
	struct rstream_tok* tok = RS_OFFSET(stream, 0);
	if (stream->head == stream->tail) { // RS_LENGTH(stream) == 0
		stream->tail++;
		tok->term = RL_TOKEN(stream);
		tok->pos = stream->lex->pos;
	}
	stream->head++;
	return tok;
}

// move (tail~head) blocks to right(n)
static void move(struct rstream *stream, int n)
{
	int len = RS_LENGTH(stream);
	if (!len || n <= 0)
		return;
	struct rstream_tok *curr = RS_OFFSET(stream, 0);
	while (len--) {
		curr[len + n] = curr[len]; // struct copy from RIGHT to LEFT
	}
	stream->head += n;
	stream->tail += n;
}

static struct rstream_tok *rstream_reduceEP(struct rstream *stream)
{
	struct rstream_tok *curr = RS_OFFSET(stream, 0);
	int pmax = (curr - 1)->pos.max;
	move(stream, 1);
	curr->pos = (struct rlex_position){pmax, pmax};
	return curr;
}

void rstream_unshift(struct rstream *stream, struct rstream_tok *src)
{
	struct rstream_tok *dst = RS_OFFSET(stream, 0);
	move(stream, 1);
	*dst = *src; // struct copy
}

// you have to update the values of (.term, .state, .pvalue), after returnning.
struct rstream_tok *rstream_reduce(struct rstream *stream, int width)
{
	if (width == 0)
		return rstream_reduceEP(stream);
	int pmax = RS_OFFSET(stream, -1)->pos.max; // save "pmax" before update stream->head
	width--;                        // reserve 1 block
	stream->head -= width;          // update stream->head
	stream->tail -= width;
	struct rstream_tok *const tok = RS_OFFSET(stream, -1); // related to the reserved block
	tok->pos.max = pmax;
	if (width) {
		// fast junk(width)
		int len = RS_LENGTH(stream);
		int i = 0;
		while (i++ < len) {       // i start at "1" because tok is -1
			tok[i] = tok[i + width];
		}
	}
	return tok;
}
