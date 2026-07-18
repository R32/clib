/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */
#include "cset.h"

static inline int max(int a, int b)
{
	return a > b ? a : b;
}

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

int cset_length(const struct cset *cs)
{
	if (!cs)
		return 0;
	const struct cset *ptr = cs;
	while (cset_not_empty(ptr))
		ptr++;
	return (int)(ptr - cs);
}

int cset_copy(struct cset *restrict dst, const struct cset *restrict src)
{
	if (!src)
		return 0;
	if (!dst)
		return cset_length(src);
	struct cset *base = dst;
	while (cset_not_empty(src))
		*dst++ = *src++;

	cset_set_terminator(dst);
	return (int)(dst - base);
}

int cset_diff(const struct cset *restrict cs, const struct cset *restrict sub, struct cset *restrict out)
{
	if (!cs || cset_is_empty(cs)) {
		if (out) cset_set_terminator(out);
		return 0;
	}
	if (!sub || cset_is_empty(sub))
		return out ? cset_copy(out, cs) : cset_length(cs);
	int cnt = 0;
	struct cset pos = *cs;
	struct cset cut = *sub;
	while (cset_not_empty(cs) && cset_not_empty(sub)) {
		if (pos.high < cut.low) {
		// ..    .|--C--|
		//                |--S--|
			cnt++;
			if (out) *out++ = pos;
			pos = *++cs;
			continue;
		}

		if (pos.low > cut.high) {
		//        |--C--|
		// |--S--|
			cut = *++sub;
			continue;
		}

		if (pos.low < cut.low) {
		//        |--C...
		//           |--S...
			cnt++;
			if (out) *out++ = (struct cset){pos.low, cut.low - 1};
		}

		if (pos.high > cut.high) {
		//        ...C--|
		//     ...S--|
			pos.low = cut.high + 1;
			cut = *++sub;
		} else {
		//        ...C--|
		//           ...S--|
			pos = *++cs;
		}
	}
	if (cset_not_empty(&pos)) {
		cnt++;
		if (out) *out++ = pos;
	}
	if (cset_not_empty(cs)) {
		while (cset_not_empty(++cs)) {
			cnt++;
			if (out) *out++ = *cs;
		}
	}
	if (out) cset_set_terminator(out);
	return cnt;
}

int cset_union(const struct cset *restrict c1, const struct cset *restrict c2, struct cset *restrict out)
{
	if (!c1 || cset_is_empty(c1))
		return out ? cset_copy(out, c2) : cset_length(c2);
	if (!c2 || cset_is_empty(c2))
		return out ? cset_copy(out, c1) : cset_length(c1);

	int cnt = 0;
	struct cset acc = c1->low < c2->low ? *c1++ : *c2++;
	while (cset_not_empty(c1) || cset_not_empty(c2)) {
		const struct cset *next;
		if (cset_not_empty(c1) && (cset_is_empty(c2) || c1->low < c2->low)) {
			next = c1++;
		} else {
			next = c2++;
		}
		if (next->low <= 1 + acc.high) {
			if (acc.high < next->high)
				acc.high = next->high;
		} else {
			if (out) *out++ = acc;
			acc = *next;
			cnt++;
		}
	}
	cnt++;
	if (out) {
		*out++ = acc;
		cset_set_terminator(out);
	}
	return cnt;
}

int cset_inter(const struct cset *restrict c1, const struct cset *restrict c2, struct cset *restrict out)
{
	int cnt = 0;
	while (cset_not_empty(c1) && cset_not_empty(c2)) {
		struct cset a = *c1;
		struct cset b = *c2;
		int start = max(a.low, b.low);
		int end = min(a.high, b.high);
		if (start <= end) {
			cnt++;
			if (out) *out++ = (struct cset){start, end};
		}
		if (a.high < b.high) {
			c1++;
		} else {
			c2++;
		}
	}
	if (out) cset_set_terminator(out);
	return cnt;
}

// Only the first element of the '@full' is used as the universal set in this function.
// Thereforce, I commented out this function and used 'cset_diff' instead.
//int cset_complement(const struct cset *restrict cs, const struct cset *restrict full, struct cset *restrict out)
//{
	// if (!full || cset_is_empty(full))
	// 	return 0;
	// if (!cs) 
	// 	cs = &(struct cset){-1, -1}; // dummy
	// int cnt = 0;
	// int left = full->low;
	// while (cset_not_empty(cs)) {
	// 	struct cset c = *cs++;
	// 	if (left < c.low) {
	// 		cnt++;
	// 		if (out) *out++ = (struct cset){left, c.low - 1};
	// 	}
	// 	if (left <= c.high) {
	// 		left = (int)c.high + 1;
	// 	}
	// }
	// int right = full->high;
	// if (left <= right && right > 0) {
	// 	cnt++;
	// 	if (out) *out++ = (struct cset){left, right};
	// }
	// if (out) cset_add_barrier(out);
	// return cnt;
//}

int cset_sort(struct cset *cs) {
	if (!cs || cset_is_empty(cs))
		return 0;
	int len = cset_length(cs);
	// bubble sort
	for (int i = 0; i < len; i++) {
		struct cset *pos = cs + i;
		for (int j = i + 1; j < len; j++) {
			struct cset *next = cs + j;
			if (pos->low <= next->low)
				continue;
			struct cset temp = *next;
			*next = *pos;
			*pos = temp;
		}
	}
	// do union
	struct cset *cover = cs;
	struct cset acc = *cs;
	for (int i = 1; i < len; i++) {
		const struct cset *next = cs + i;
		if (next->low <= 1 + acc.high) {
			if (acc.high < next->high)
				acc.high = next->high;
		} else {
			*cover++ = acc;
			acc = *next;
		}
	}
	*cover++ = acc;
	cset_set_terminator(cover);
	return (int)(cover - cs);
}
