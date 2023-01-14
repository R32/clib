#ifndef R_ARRAY_H
#define R_ARRAY_H

#include "rclibs.h"
#include <stdlib.h>
#include <string.h>

#ifndef ra_realloc
#	define ra_realloc realloc
#endif
#ifndef ra_free
#	define ra_free free
#endif

struct _rarray_base;
typedef struct _rarray_base *prarray_base;

struct rarray {
	prarray_base base;
	int size; // elemsize
};

#define rarray_fast_get(prar, type, i)    (((type *)(prar)->base) + (i))
#define rarray_fast_set(prar, type, i, v) (*rarray_fast_get(prar, type, i) = *(v))

C_FUNCTION_BEGIN

void rarray_init(struct rarray *prar, int elemsize);

// Release "prar->base" but "prar" can still be reused
void rarray_discard(struct rarray *prar);

// Increase capacity only
void rarray_grow(struct rarray *prar, int cap);

// Set "len" and increment "cap" if exceeded
void rarray_setlen(struct rarray *prar, int len);

int rarray_len(struct rarray *prar);
int rarray_cap(struct rarray *prar);

int rarray_push(struct rarray *prar, void *value);
void *rarray_pop(struct rarray *prar);
void *rarray_get(struct rarray *prar, int index);
void rarray_set(struct rarray *prar, int index, void *value);

C_FUNCTION_END
#endif
