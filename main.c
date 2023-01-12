#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "rclibs.h"
#include "slist.h"
#include "circ_buf.h"
#include "list.h"
#include "rbtree_augmented.h"
#include "ucs2.h"
#include "tinyalloc.h"

struct blk_s {
	int n;
	struct slist_head link;
	struct rb_node rb;
};

static void t_ucs2() {
	unsigned char utf8_copy[32];
	unsigned char utf8[] =
		"\xE4\xB8\x87\xE8\x88\xAC\xE7\x9A\x86\xE4\xB8"
		"\x8B\xE5\x93\x81\x2C\x20\xF0\xA8\xB0\xBB\x41";
	wchar_t ucs2_copy[32];
	wchar_t ucs2[] = L"\x4e07\x822c\x7686\x4e0b\x54c1"  L", \xd863\xdc3b"  L"A";
	// wcs_to_utf8
	int bytes = ARRAYSIZE(utf8) - 1; // strip '\0'
	assert(wcstoutf8(NULL     , ucs2) == bytes);
	assert(wcstoutf8(utf8_copy, ucs2) == bytes);
	assert(memcmp(utf8_copy, utf8, bytes) == 0);
	// utf8_to_wcs
	int wslen = ARRAYSIZE(ucs2) - 1;
	assert(utf8towcs(NULL     , utf8) == wslen);
	assert(utf8towcs(ucs2_copy, utf8) == wslen);
	assert(memcmp(ucs2_copy, ucs2, wslen * sizeof(wchar_t)) == 0);
}

static void t_slist() {
	// MSVC doesn't support VLA
	#define SLEN 10
	int a[SLEN];
	struct slist_head head = SLIST_HEAD_INIT;
	int i = 0;
	for(; i < SLEN; i++) {
		struct blk_s* data = malloc(sizeof(struct blk_s));
		data->n = i * i;
		a[i] = i * i;
		slist_add(&data->link, &head);
	}
	assert(slist_len(&head) == SLEN);

	struct slist_head* pos; // for loop
	slist_for_each(pos, &head) {
		struct blk_s* data = slist_entry(pos, struct blk_s, link);
		assert(data->n == a[--i]);
	}

	assert(i == 0);

	slist_rev(&head);
	slist_for_each(pos, &head) {
		struct blk_s* data = slist_entry(pos, struct blk_s, link);
		assert(data->n == a[i++]);
	}
	i = 0;
	struct blk_s* data = slist_entry( slist_pop(&head), struct blk_s, link ); // popup
	assert(data->n == a[i++] && slist_len(&head) == SLEN - 1);
	free(data);
	while(slist_first(&head)) {
		data = slist_entry( slist_pop(&head), struct blk_s, link );
		assert(data->n == a[i++]);
		free(data);
	}
	assert(slist_len(&head) == 0);
}
// rbtree search
static struct blk_s* rbSearch(struct rb_root *root, int n) {
	struct rb_node *node = root->rb_node;
	while (node) {
		struct blk_s* cur = rb_entry(node, struct blk_s, rb);
		int32_t result = n - cur->n;
		if (result < 0) {
			node = node->rb_left;
		} else if (result > 0) {
			node = node->rb_right;
		} else {
			return cur;
		}
	}
	return NULL;
}
// rbtree insert
static bool rbInsert(struct blk_s *data, struct rb_root *root) {
	struct rb_node **new = &(root->rb_node);
	struct rb_node *parent = NULL;
	while (*new) {
		struct blk_s* cur = rb_entry(*new, struct blk_s, rb);
		int result = data->n - cur->n;
		parent = *new;
		if (result < 0) {
			new = &((*new)->rb_left);
		} else if (result > 0) {
			new = &((*new)->rb_right);
		} else {
			return false;
		}
	}
	/* Add new node and rebalance tree. */
	rb_link_node(&data->rb, parent, new);
	rb_insert_color(&data->rb, root);
	return true;
}

static void t_rbtree() {
	struct rb_root root = RB_ROOT;
	// insert
	for (int i = 0; i < 10; i++) {
		struct blk_s* data = malloc(sizeof(struct blk_s));
		data->n = i;
		data->link.next = NULL;
		rbInsert(data, &root); // <<<<<<
	}

	// Iteration
	int i = 0;
	for (struct rb_node* node = rb_first(&root); node; node = rb_next(node)) {
		struct blk_s* data = rb_entry(node, struct blk_s, rb);
		assert(data->n == i++);
	}

	// Search
	struct blk_s* data = rbSearch(&root, 2);
	assert(data && data->n == 2);

	// Safe Iteration & Delete
	struct rb_node* node = rb_first(&root);
	struct rb_node* next = NULL;
	while (node) {
		next = rb_next(node);
		rb_erase(node, &root);
		////
		struct blk_s* data = rb_entry(node, struct blk_s, rb);
		free(data);
		node = next;
	}
	assert(rb_first(&root) == NULL);
}

void shuffle(void* a[], int len) {
	char* tmp = NULL;
	int t;
	for(int i =0; i < len; i++) {
		t = rand() % len;
		tmp = a[t];
		a[t] = a[i];
		a[i] = tmp;
	}
}

#define BLKMAX             1024
#define PTRSIZE(ptr)       (*(((int*)(ptr)) - 1))
#define BLK_BASE           (TINYALLOC_BLK_BASE)
#define IS_ALIGNED(ptr)    (((size_t)(ptr) & (BLK_BASE - 1)) == 0)

int ptr_intersect(const void* aa, const void* bb) {
	char* a = *(char**)aa;
	char* b = *(char**)bb;

	assert(IS_ALIGNED(a));
	assert(IS_ALIGNED(b));

	assert(PTRSIZE(a) > BLK_BASE);
	assert(PTRSIZE(b) > BLK_BASE);

	for (int i = 0; i < PTRSIZE(a); i++) {
		assert(a[i] == 'X');
	}
	for (int i = 0; i < PTRSIZE(b); i++) {
		assert(b[i] == 'X');
	}

	if (a > b) {
		assert(b + PTRSIZE(b) < a);
	} else if (a < b) {
		assert(a + PTRSIZE(a) < b);
	} else {
		assert(0);
	}
	return a - b;
}
void *__alloc_x(struct tinyalloc_root *root, int size) {
	char *ptr = tinyalloc(root, size);
	if (ptr == NULL)
		assert(0);
	size = PTRSIZE(ptr);
	for (int i = 0; i < size; i++)
		ptr[i] = 'X';
	return ptr;
}
void t_tinyalloc() {
	srand((uint32_t)time(NULL));
	struct tinyalloc_root root = {NULL};

	// randomly alloc and free
	#define RAND()        (rand() & (BLKMAX - 1))
	#define __alloc(size) __alloc_x(&root, size)
	#define __free(ptr)   tinyfree(&root, ptr)
	#define ASIZE         (960)
	#define HALFASIZE     (ASIZE >> 1)
	#define KB(k)         (1024 * (k))
	char* aptr[ASIZE];
	int big[] = {KB(88), KB(101), KB(196), KB(99), KB(256), KB(201), KB(512), KB(333), KB(222)};
	int i;
	for (i = 0; i < ASIZE - ARRAYSIZE(big); i++) {
		aptr[i] = __alloc(RAND());
	}
	for (int j = 0; j < ARRAYSIZE(big); j++) {
		aptr[i + j] = __alloc(big[j]);
	}
	assert(slist_len(&root.chunk_head) > 0);

	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), ptr_intersect);
	shuffle((void**)aptr, ASIZE);

	// HALFASIZE free & alloc
	for(i = 0; i < HALFASIZE; i++) __free(aptr[i]);
	for(i = 0; i < HALFASIZE; i++) aptr[i] = __alloc(RAND());

	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), ptr_intersect);
	shuffle((void**)aptr, ASIZE);
	// another HALFSIZE
	for(i = HALFASIZE; i < ASIZE; i++) __free(aptr[i]);
	for(i = HALFASIZE; i < ASIZE; i++) aptr[i] = __alloc(RAND());

	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), ptr_intersect);

	tinyreset(&root);
	tinydestroy(&root);
	assert(slist_empty(&root.chunk_head));
}

int bump_intersect(const void* aa, const void* bb)
{
	char* a = *(char**)aa;
	char* b = *(char**)bb;
	assert(IS_ALIGNED(a));
	assert(IS_ALIGNED(b));
	if (a > b) {
		assert(b + (*(int*)b) <= a);
	} else if (a < b) {
		assert(a + (*(int*)a) <= b);
	} else {
		assert(0);
	}
	return a - b;
}
void *__alloc_y(struct bumpalloc_root *bump, int size)
{
	if (size < BLK_BASE) {
		size = BLK_BASE;
	} else {
		size = ALIGN_POW2(size, BLK_BASE);
	}
	char *ptr = bumpalloc(bump, size);
	if (ptr == NULL)
		assert(0);
	*(int*)ptr = size; // write size to ptr
	return ptr;
}
void t_bumpalloc()
{
	srand((uint32_t)time(NULL));
	struct bumpalloc_root bump = { NULL };
	#define RAND()        (rand() & (BLKMAX - 1))
	#undef __alloc
	#define __alloc(size) __alloc_y(&bump, size)
	#define ASIZE         (960)
	char* aptr[ASIZE];
	int big[] = { KB(88), KB(101), KB(196), KB(99), KB(256), KB(201), KB(512), KB(333), KB(222) };
	int i;
	for (i = 0; i < ASIZE - ARRAYSIZE(big); i++) {
		aptr[i] = __alloc(RAND());
	}
	for (int j = 0; j < ARRAYSIZE(big); j++) {
		aptr[i + j] = __alloc(big[j]);
	}
	assert(slist_len(&bump.chunk_head) > 0);

	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), bump_intersect);

	bumpreset(&bump);
	for (i = 0; i < ASIZE; i++) {
		aptr[i] = __alloc(RAND());
	}
	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), bump_intersect);

	bumpdestroy(&bump);
	assert(slist_empty(&bump.chunk_head));
}

int fixed_intersect(const void* aa, const void* bb)
{
	char* a = *(char**)aa;
	char* b = *(char**)bb;
	assert(IS_ALIGNED(a));
	assert(IS_ALIGNED(b));
	if (a > b) {
		assert(b + 512 <= a);
	} else if (a < b) {
		assert(a + 512 <= b);
	} else {
		assert(0);
	}
	return a - b;
}
void t_fixedalloc()
{
	#define ASIZE         (960)
	struct fixedalloc_root fixed;
	fixedalloc_init(&fixed, 512);
	char* aptr[ASIZE];
	for (int i = 0; i < ASIZE; i++) {
		aptr[i] = fixedalloc(&fixed);
	}
	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), fixed_intersect);

	int len = slist_len(&fixed.chunk_head);
	for (int i = 0; i < ASIZE / 2; i++) fixedfree(&fixed, aptr[i]);
	for (int i = 0; i < ASIZE / 2; i++) aptr[i] = fixedalloc(&fixed);
	assert(len > 0 && len == slist_len(&fixed.chunk_head));
	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), fixed_intersect);

	fixedreset(&fixed);
	for (int i = 0; i < ASIZE; i++) {
		aptr[i] = fixedalloc(&fixed);
	}
	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), fixed_intersect);

	fixeddestroy(&fixed);
	assert(slist_empty(&fixed.chunk_head));
}

int main(int argc, char** args) {
	setlocale(LC_CTYPE, "");
	t_ucs2();
	t_slist();
	t_rbtree();
	for (int i = 0; i < 7; i++) {
		t_tinyalloc();
		t_bumpalloc();
		t_fixedalloc();
	}
	printf("done!\n");
	return 0;
}
