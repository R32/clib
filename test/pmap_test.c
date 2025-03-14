#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "pmap.h"

struct data {
	int key;
	int value;
	struct pmnode node;
};

static struct data *data_search(struct pmnode *root, int key)
{
	struct pmnode *pos = root;
	while (pos) {
		// <------
		struct data *curr = container_of(pos, struct data, node);
		int cmp = key - curr->key;
		// <------
		if (cmp == 0)
			return curr;
		pos = cmp < 0 ? pos->left : pos->right;
	}
	return NULL;
}

static struct data *data_remove(struct pmnode **root, int key)
{
	int index = -1;
	pmap_stacks_decl(pmap_stacks, pmap_height(*root));
	struct pmnode **slot = root;
	while (*slot) {
		// <------
		struct data *curr = container_of(*slot, struct data, node);
		int cmp = key - curr->key;
		// <------
		if (cmp == 0)
			break;
		pmap_stacks[++index] = slot;

		if (cmp < 0) {
			slot = &(*slot)->left;
		} else {
			slot = &(*slot)->right;
		}
	}
	struct pmnode *victim = *slot;
	if (victim == NULL)
		return NULL;
	pmap_merge(slot);
	while (index >= 0) {
		pmap_balance(pmap_stacks[index--], &index);
	}
	// <------
	return container_of(victim, struct data, node);
	// <------
}

static struct data *data_insert(struct pmnode **root, struct data *data)
{
	int index = -1;
	pmap_stacks_decl(pmap_stacks, pmap_height(*root));
	struct pmnode **slot = root;
	while (*slot) {
		// <------
		struct data *curr = container_of(*slot, struct data, node);
		int cmp = data->key - curr->key;
		// <------
		pmap_stacks[++index] = slot;
		if (cmp < 0) {
			slot = &(*slot)->left;
		} else if (cmp > 0) {
			slot = &(*slot)->right;
		} else {
			return curr; // Fails if already exists
		}
	}
	// init new node
	data->node = (struct pmnode){.left = NULL, .right = NULL, .height = 1};

	// link node to the NULL place.
	*slot = &data->node;

	// do balancing
	while (index >= 0) {
		pmap_balance(pmap_stacks[index--], &index);
	}
	return NULL;
}

static int data_print(char *desc, int deep, struct pmnode *node) {
	if (!node)
		return 0;
	struct data *data = container_of(node, struct data, node);
	int i = data_print("L", deep + 4, node->left);
	printf("%*s[%d h(%d)]\n", deep, desc, data->key, data->node.height);
	i++;
	i += data_print("R", deep + 4, node->right);
	return i;
}

static void data_iter_forward(struct pmnode *root) {
	// <------ for test
	static int prev = -1;
	// <------
	struct pmnode *node = root;
	if (!node)
		return;

	int index = -1;
	VLADecl(struct pmnode*, pmnode_stacks, pmap_height(root) + 1);
	// entry_first(leftmost)
	while (node->left) {
		pmnode_stacks[++index] = node;
		node = node->left;
	}
	// entry_next
	while (1) {
		// <--------
		struct data *data = container_of(node, struct data, node);
		assert(prev + 1 == data->key);
		prev = data->key;
		// <--------
		if (node->right) {
			node = node->right;
			while (node->left) {
				pmnode_stacks[++index] = node;
				node = node->left;
			}
			continue;
		}
		if (index < 0)
			break;
		node = pmnode_stacks[index--];
	}

	// <------ reset test
	prev = -1;
	// <------
}

#ifndef SIZE
#   define SIZE (16 * 8192)
#endif

static void test_inner(struct data *pdata, int logout)
{
	// shuffle
	struct data tmp;
	for (int i = 0; i < SIZE; ++i) {
		int r = rand() % SIZE;
		tmp = pdata[r];
		pdata[r] = pdata[i];
		pdata[i] = tmp;
	}

	// init pmap root
	struct pmnode *root = NULL;

	if (logout) printf("------------\n");
	// inserting
	clock_t t = clock();
	for (int i = 0; i < SIZE; ++i) {
		struct data *error = data_insert(&root, &pdata[i]);
		assert(error == NULL);
	}
	t = clock() - t;
	if (logout) printf("Inserting count : %d, time : %.6f, tree height : %d\n", SIZE, ((double)t) / CLOCKS_PER_SEC, root->height);

	// data_iter("*", 1, root);
	data_iter_forward(root);
	assert(pmap_count(root) == SIZE);

	// finding
	t = clock();
	for (int i = 0; i < SIZE; ++i) {
		struct data *data = &pdata[i];
		struct data *find = data_search(root, data->key);
		assert(data == find);
	}
	t = clock() - t;
	if (logout) printf("  Finding count : %d, time : %.6f\n", SIZE, ((double)t) / CLOCKS_PER_SEC);

	// Removing
	t = clock();
	for (int i = 0; i < SIZE; ++i) {
		struct data *data = data_remove(&root, pdata[i].key);
		assert(data == &pdata[i]);
	}
	t = clock() - t;
	if (logout) printf(" Removing count : %d, time : %.6f\n", SIZE, ((double)t) / CLOCKS_PER_SEC);

	assert(root == NULL);
}

void pmap_test(int n)
{
	struct data *pdata = malloc(SIZE * sizeof(struct data));
	for (int i = 0; i < SIZE; ++i) {
		struct data *data = &pdata[i];
		data->key = i;
		data->value = i + i;
	}

	for (int i = 0; i < n; i++) {
		test_inner(pdata, 0);
	}
	free(pdata);
}
