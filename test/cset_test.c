#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <assert.h>
#include "cset.h"

static void eq(struct cset *c1, struct cset *c2)
{
	int len = cset_length(c1);
	assert(len == cset_length(c2));
	for (int i = 0; i < len; i++) {
		assert(*(int *)(c1 + i) == *(int *)(c2 + i));
	}
}
static char *cset_string(struct cset *cs, char *buff, int size)
{
	if (size-- > 0) *buff++ = '{';
	char *base = buff;
	while (cset_not_empty(cs)) {
		int n = snprintf(buff, size, "[%d, %d], ", cs->low, cs->high);
		buff += n;
		size -= n;
		cs++;
	}
	if (base != buff) {
		buff -= 2; // remove ', '
		size += 2;
	}
	if (size-- > 0) *buff++ = '}';
	*buff = 0;
	return base;
}

static struct cset CNUL = { -1, -1 };

static void union_test()
{
	struct cset out[64];
	{
		struct cset c1[] = { {32, 32}, {51, 59}, {65, 65}, {97, 102}, CNUL };
		struct cset c2[] = { {10, 10}, {51, 59}, {65, 70}, CNUL };
		struct cset cr[] = { {10, 10}, {32, 32}, {51, 59}, {65, 70}, {97, 102}, CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { {1, 5}, CNUL };
		struct cset cr[] = { {1, 5}, CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}
	
	{
		struct cset c1[] = { {1, 5}, {10, 15}, {20, 25}, CNUL };
		struct cset c2[] = { {3, 12}, {16, 22}, CNUL };
		struct cset cr[] = { {1, 25}, CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 5}, {10, 15}, {20, 25}, CNUL };
		struct cset c2[] = { {6, 9}, {16, 19}, CNUL };
		struct cset cr[] = { {1, 25}, CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 3}, {5, 7}, {9, 11}, CNUL };
		struct cset c2[] = { {4, 4}, {8, 8}, {12, 12}, CNUL };
		struct cset cr[] = { {1, 12}, CNUL };
		assert(cset_union(c1, c2, NULL) == cset_length(cr));
		assert(cset_union(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}
}

static void inter_test()
{
	struct cset out[64];
	{
		struct cset c1[] = { {32, 32}, {51, 59}, {65, 65}, {97, 102}, CNUL };
		struct cset c2[] = { {10, 10}, {51, 59}, {65, 70}, CNUL };
		struct cset cr[] = { {51, 59}, {65, 65}, CNUL };
		assert(cset_inter(c1, c2, NULL) == cset_length(cr));
		assert(cset_inter(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { {1, 5}, CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_inter(c1, c2, NULL) == cset_length(cr));
		assert(cset_inter(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_inter(c1, c2, NULL) == cset_length(cr));
		assert(cset_inter(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 10}, {20, 30}, CNUL };
		struct cset c2[] = { {5, 25}, CNUL };
		struct cset cr[] = { {5, 10}, {20, 25}, CNUL };
		assert(cset_inter(c1, c2, NULL) == cset_length(cr));
		assert(cset_inter(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 5}, {10, 15}, {20, 25}, CNUL };
		struct cset c2[] = { {3, 12}, {16, 22}, CNUL };
		struct cset cr[] = { {3, 5}, {10, 12}, {20, 22}, CNUL };
		assert(cset_inter(c1, c2, NULL) == cset_length(cr));
		assert(cset_inter(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}
}

static void diff_test()
{
	struct cset out[64];
	{
		struct cset c1[] = { {32, 32}, {51, 59}, {65, 65}, {97, 102}, CNUL };
		struct cset c2[] = { {10, 10}, {51, 59}, {65, 70}, CNUL };
		struct cset cr[] = { {32, 32}, {97, 102}, CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { {1, 5}, CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 5}, CNUL };
		struct cset c2[] = { CNUL };
		struct cset cr[] = { {1, 5}, CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { CNUL };
		struct cset c2[] = { CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 10}, {20, 30}, CNUL };
		struct cset c2[] = { {5, 25}, CNUL };
		struct cset cr[] = { {1, 4}, {26, 30}, CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {1, 5}, {10, 15}, {20, 25}, CNUL };
		struct cset c2[] = { {3, 12}, {16, 22}, CNUL };
		struct cset cr[] = { {1, 2}, {13, 15}, {23, 25}, CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset c1[] = { {32, 32}, {34, 51}, {65, 70}, CNUL };
		struct cset c2[] = { {33, 33}, {37, 37}, {41, 59}, {61, 63}, {65, 67}, CNUL };
		struct cset cr[] = { {32, 32}, {34, 36}, {38, 40}, {68, 70}, CNUL };
		assert(cset_diff(c1, c2, NULL) == cset_length(cr));
		assert(cset_diff(c1, c2, out) == cset_length(cr));
		eq(cr, out);
	}
}

static void complement_test()
{
	struct cset out[64];
	{
		struct cset set[] = { {32, 32}, {51, 59}, {65, 65}, {97, 102}, CNUL };
		struct cset full[] = { {0, 127}, CNUL };
		struct cset cr[] = { {0, 31}, {33, 50}, {60, 64}, {66, 96}, {103, 127}, CNUL };
		assert(cset_complement(set, full, NULL) == cset_length(cr));
		assert(cset_complement(set, full, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset set[] = { CNUL };
		struct cset full[] = { {0, 255}, CNUL };
		struct cset cr[] = { {0, 255}, CNUL };
		assert(cset_complement(set, full, NULL) == cset_length(cr));
		assert(cset_complement(set, full, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset set[] = { {0, 255}, CNUL };
		struct cset full[] = { {0, 255}, CNUL };
		struct cset cr[] = { CNUL };
		assert(cset_complement(set, full, NULL) == cset_length(cr));
		assert(cset_complement(set, full, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset set[] = { {10, 20}, {30, 40}, {50, 60}, CNUL };
		struct cset full[] = { {0, 100}, CNUL };
		struct cset cr[] = { {0, 9}, {21, 29}, {41, 49}, {61, 100}, CNUL };
		assert(cset_complement(set, full, NULL) == cset_length(cr));
		assert(cset_complement(set, full, out ) == cset_length(cr));
		eq(cr, out);
	}

	{
		struct cset set[] = { {5, 15}, {25, 35}, {45, 55}, CNUL };
		struct cset full[] = { {10, 50}, CNUL };
		struct cset cr[] = { {16, 24}, {36, 44}, CNUL };
		assert(cset_complement(set, full, NULL) == cset_length(cr));
		assert(cset_complement(set, full, out ) == cset_length(cr));
		eq(cr, out);
	}
}

static void sort_test()
{
	struct cset copy[32];
	struct cset cs[] = { {6, 8}, {1, 5}, {10, 13}, {12, 15}, {22, 22}, CNUL };
	struct cset cr[] = { {1, 8}, {10, 15}, {22, 22}, CNUL };
	assert(cset_copy(copy, cs) == cset_length(cs));
	eq(cs, copy);
	cset_sort(cs);
	eq(cs, cr);
}


void cset_test()
{
	sort_test();
	diff_test();
	union_test();
	inter_test();
	complement_test();
}
