/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 * Copyright (C) 2025 LWM
 *
 * To use pmap you'll have to implement your own insert, remove, search and iterater routines.
 * This will avoid us to use callbacks and to drop drammatically performances.
 * I know it's not the cleaner way, but in C (not in C++) to get performances and genericity...
 *
 * Refer to `test/pmap_test.c` for samples.
 */

#include "pmap.h"

static int inline imax(int a, int b)
{
	return a > b ? a : b;
}

int pmap_count(struct pmnode *root)
{
	if (!root)
		return 0;
	int n = 1;
	n += pmap_count(root->left);
	n += pmap_count(root->right);
	return n;
}

void pmap_balance(struct pmnode ***stacks, int index)
{
	while (index >= 0) {
		struct pmnode **slot = stacks[index--];
		struct pmnode *node = *slot;
		struct pmnode *left = node->left;
		struct pmnode *right = node->right;
		int hL = pmap_height(left);
		int hR = pmap_height(right);
		if (hL > hR + 2) {
			int hLL = pmap_height(left->left);
			int hLR = pmap_height(left->right);
			if (hLL >= hLR) {
				/*      N     ->        L
				 *    L   R   ->     LL    N
				 * (LL LR)    ->        (LR  R)
				 */
				node->left = left->right;
				node->height = imax(hLR, hR) + 1;

				left->right = node;
				left->height = imax(hLL, node->height) + 1;

				*slot = left;
			} else {
				/*      N              LR
				 *    L   R        L        N
				 *  LL LR       (LL LRL) (LRR R)
				 *   (LRL LRR)
				 */
				struct pmnode *LR = left->right;

				left->right = LR->left;
				left->height = imax(hLL, pmap_height(LR->left)) + 1;

				node->left = LR->right;
				node->height = imax(pmap_height(LR->right), hR) + 1;

				LR->left = left;
				LR->right = node;
				LR->height = imax(left->height, node->height) + 1;

				*slot = LR;
			}
		} else if (hR > hL + 2) {
			int hRL = pmap_height(right->left);
			int hRR = pmap_height(right->right);
			if (hRR >= hRL) {
				/*
				 *    N              R
				 *  L   R          N   RR
				 *    (RL RR)   (L  RL)
				 */
				node->right = right->left;
				node->height = imax(hL, hRL) + 1;

				right->left = node;
				right->height = imax(node->height, hRR) + 1;

				*slot = right;
			} else {
				/*     N                RL
				 *   L   R          N        R
				 *    (RL RR)    (L RLL) (RLR RR)
				 * (RLL RLR)
				 */
				struct pmnode *RL = right->left;

				node->right = RL->left;
				node->height = imax(hL, pmap_height(RL->left)) + 1;

				right->left = RL->right;
				right->height = imax(pmap_height(RL->right), hRR) + 1;

				RL->left = node;
				RL->right = right;
				RL->height = imax(node->height, right->height) + 1;

				*slot = RL;
			}
		} else {
			int height = imax(hL, hR) + 1;
			if (height == node->height && height > 1)
				return;
			node->height = height;
		}
	}
}

void pmap_remove(struct pmnode **slot, struct pmnode ***stacks, int index)
{
	struct pmnode *vic = *slot; // a node is being removed
	struct pmnode *left = vic->left;
	struct pmnode *right = vic->right;
	if (left == NULL) {
		*slot = right;
		goto balance;
	} else if (right == NULL) {
		*slot = left;
		goto balance;
	}

	/*    V             R
	 *  L   R   ->    L   RR
	 *   (NULL  RR)
	 */
	if (right && right->left == NULL) {
		*slot = right;
		right->left = left;
		right->height = imax(left->height, pmap_height(right->right)) + 1;
		goto balance;
	}

	/*
	 *      V                  LM
	 *   L     R            L      R
	 *       RL  RR    ->       RL   RR
	 *    (RLL -)            (RLL -)
	 * (LM  -)             (LMR  -)
	 *    LMR
	 */
	int offset = index;
	stacks[++index] = slot;
	struct pmnode **anchor = &vic->right;
	while (*anchor) {
		stacks[++index] = anchor;
		anchor = &(*anchor)->left;
	}
	// popup leftmost node
	struct pmnode *node = *stacks[index--];

	// leftmost_parent->left = leftmost->right; (remove_min_binding)
	(*stacks[index])->left = node->right;

	// copy (.left, .right, .height)
	node->left = left;
	node->right = right;
	node->height = vic->height;

	// link the leftmost node to the slot
	*slot = node;
	// update &slot->right to &node->right in stacks ([+1] => slot, [+2] => &slot->right)
	stacks[offset + 2] = &node->right;

balance:
	pmap_balance(stacks, index);
}
