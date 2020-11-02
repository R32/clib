#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comm.h"
#include "slist.h"
#include "assert.h"
#include "circ_buf.h"
#include "list.h"
#include "rbtree_augmented.h"
#include "ucs2.h"

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
	#define len 10
	int a[len];
	struct slist_head head = SLIST_HEAD_INIT;
	int i = 0;
	for(; i < len; i++) {
		struct blk_s* data = malloc(sizeof(struct blk_s));
		data->n = i * i;
		a[i] = i * i;
		slist_add(&data->link, &head);
	}
	assert(slist_len(&head) == len);

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
	assert(data->n == a[i++] && slist_len(&head) == len - 1);
	free(data);
	while(!slist_empty(&head)) {
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

int main(int argc, char** args) {
	setlocale(LC_CTYPE, "");
	t_ucs2();
	t_slist();
	t_rbtree();
	return 0;
}
