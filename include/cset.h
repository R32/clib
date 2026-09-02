/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#ifndef LWM_CSET_H
#define LWM_CSET_H

#ifndef CSET_BASETYPE
#define CSET_BASETYPE  unsigned short
#define CSET_BASEBITS  16
#define CSET_MEGETYPE  int
#endif

/*
 * To keep it simple, I made it kind of like string, except that here `-1` is used as the terminator.
 *
 * All arguemnts of type `struct cset*`(except 'out') must end with `-1`.
 *
 * All arguments(except 'out') must not be NULL.
 *
 * If 'out' is NULL, the function only returns the length required for 'out' (not including the '-1' terminator).
 */
struct cset {
	CSET_BASETYPE low;
	CSET_BASETYPE high;
};

#define cset_not_empty(cs)        (*(CSET_MEGETYPE *)(cs) != (CSET_MEGETYPE)-1)
#define cset_is_empty(cs)         (*(CSET_MEGETYPE *)(cs) == (CSET_MEGETYPE)-1)
#define cset_is_single(cs)        ((cs)->low == (cs)->high)
#define cset_set_single(cs, v)    (*(CSET_MEGETYPE *)(cs) = (v) << CSET_BASEBITS | (v))
#define cset_set_terminator(cs)   (*(CSET_MEGETYPE *)(cs) = -1)

int cset_length(const struct cset *cs);

int cset_copy(struct cset *restrict dst, const struct cset *restrict src);

/*
 * returns the length of the cset added to out (excluding the terminator).
 */
int cset_union(const struct cset *restrict c1, const struct cset *restrict c2, struct cset *restrict out);
int cset_inter(const struct cset *restrict c1, const struct cset *restrict c2, struct cset *restrict out);
int cset_diff (const struct cset *restrict cs, const struct cset *restrict sub, struct cset *restrict out);
#define cset_complement(cs, full, out) cset_diff(full, cs, out)

/*
 * do sort and union on 'cs'
 */
int cset_sort(struct cset *cs);

#endif
