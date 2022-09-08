#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "comm.h"
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
	int bytes = ARRAY_SIZE(utf8) - 1; // strip '\0'
	assert(wcstoutf8(NULL     , ucs2) == bytes);
	assert(wcstoutf8(utf8_copy, ucs2) == bytes);
	assert(memcmp(utf8_copy, utf8, bytes) == 0);
	// utf8_to_wcs
	int wslen = ARRAY_SIZE(ucs2) - 1;
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
// copeid from tinyalloc.c for test
struct meta {
	int datasize;
	void* data[0];
};
struct chunk {
	int mempos;
	int memsize;
	int frags;
	int blocks;
	struct meta* freelist[8 + 1];
	struct slist_head link;
	char mem[0];
};
#define PTRSIZE(ptr)       (*(((int*)(ptr)) - 1))
int ptr_intersect(const void* aa, const void* bb) {
	char* a = *(char**)aa;
	char* b = *(char**)bb;
	assert(PTRSIZE(a) <= 256 && PTRSIZE(b) <= 256);
	if (a > b) {
		assert(b + PTRSIZE(b) < a);
	} else if (a < b) {
		assert(a + PTRSIZE(a) < b);
	} else {
		assert(0);
	}
	return a - b;
}
void chk_stat_print(struct chunk* chk) {
	printf("used: %3d%%, frags: %5d, blocks: %5d\n",
	  (int)((float)chk->mempos / chk->memsize * 100),
	  chk->frags,
	  chk->blocks
	);
}
void t_tinyalloc() {
	srand((uint32_t)time(NULL));
	SLIST_HEAD(root);
	int line = 64 * 1024 - sizeof(struct chunk); //
	int blocks = 0;
	int piece = 16;
	int meta_piece = piece + sizeof(struct meta);
	while (line >= meta_piece) {
		tinyalloc(&root, piece);
		line -= meta_piece;
		blocks++;
	}
	assert(slist_singular(&root));
	struct chunk* chk = slist_first_entry(&root, struct chunk, link);
	assert(chk->mempos == chk->memsize - line);
	assert(chk->blocks == blocks);
	// discard directly
	tinydestroy(&root);
	assert(slist_empty(&root));

	// randomly alloc and free
#	define RAND() (rand() & (256-1))
#	define __alloc(size) tinyalloc(&root, size)
#	define __free(ptr) tinyfree(&root, ptr)
#	define ASIZE     960
#	define HALFASIZE (ASIZE >> 1)

	char* aptr[ASIZE];
	int i;
	for(i = 0; i < ASIZE; i++) {
		aptr[i] = __alloc(RAND());
	}
	shuffle((void**)aptr, ASIZE);
	qsort(aptr, ASIZE, sizeof(aptr[0]), ptr_intersect);
	shuffle((void**)aptr, ASIZE);

	struct slist_head* pos;
	slist_for_each(pos, &root) {
		chk = slist_entry(pos, struct chunk, link);
		assert(chk->frags == 0);
	}
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
	shuffle((void**)aptr, ASIZE);
	//slist_for_each(pos, &root) {
	//	chk = slist_entry(pos, struct chunk, link);
	//	chk_stat_print(chk);
	//}
	for(i = 0; i < ASIZE; i++) {
		__free(aptr[i]);
	}
	char* bigptr = __alloc(1024 * 88);
	assert(bigptr);
	__free(bigptr);
	slist_for_each(pos, &root) {
		chk = slist_entry(pos, struct chunk, link);
		assert(chk->mempos == 0 && chk->frags == 0 && chk->blocks == 0);
	}
	tinydestroy(&root);
	assert(slist_empty(&root));
}
int main(int argc, char** args) {

	setlocale(LC_CTYPE, "");
	t_ucs2();
	t_slist();
	t_rbtree();
	for (int i = 0; i < 100; i++) {
		t_tinyalloc();
	}
	return 0;
}
