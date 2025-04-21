/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LWM_BUFFER_H
#define LWM_BUFFER_H

struct buffer {
	struct chunk *head;
	struct chunk *tail;
};

struct chunk {
	int pos;
	int len;
	union {
		struct chunk *next;
		double _x; // padding to 8 bytes for 32-bit systems
	} list;
	unsigned char data[];
};

#ifndef FATAL_OUT_OF_MEMORY
#   define FATAL_OUT_OF_MEMORY(ptr)
#endif

#ifndef CHK_MALLOC
#   include <stdlib.h>
#   define CHK_ALLOC     malloc
#   define CHK_FREE      free
#endif

#define CHK_NEXT(chk)    ((chk)->list.next)

#define buffer_for_each(buf, chk) \
	for (struct chunk *chk = (buf)->head; chk; chk = chk->list.next)


int buffer_length(struct buffer *buf);
void buffer_reset(struct buffer *buf);
void buffer_release(struct buffer *buf);

/*
 * BEWARE : make sure that the 'buf' is initialized
 */
struct chunk *buffer_append_chunk(struct buffer *buf, int len, int size);

#endif