/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 * Copyright (C) 2025 Liuwm
 */
/*
 * PMap - Polymorphic maps
 * Copyright (C) 1996-2003 Xavier Leroy, Nicolas Cannasse, Markus Mottl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version,
 * with the special exception on linking described in file LICENSE.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
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

void pmap_balance(struct pmnode **slot, int *breakout)
{
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
		if (height == node->height) {
			*breakout = -1; // to break the outside loop
			return;
		}
		node->height = height;
	}
}

struct pmnode *pmap_merge(struct pmnode **slot)
{
	struct pmnode *victim = *slot;
	struct pmnode *left = victim->left;
	struct pmnode *right = victim->right;

	if (left == NULL) {
		*slot = right;
		return victim;
	} else if (right == NULL) {
		*slot = left;
		return victim;
	}

	/*    N             R
	 *  L   R   ->    L   RR
	 *   (NULL  RR)
	 */
	if (right && right->left == NULL) {
		*slot = right;
		right->left = left;
		right->height = imax(left->height, pmap_height(right->right)) + 1;
		return victim;
	}

	/*
	 *      N                  LM
	 *   L     R            L      R
	 *       RL  RR    ->       RL   RR
	 *    (RLL -)            (RLL -)
	 * (LM  -)             (LMR  -)
	 *    LMR
	 */
	int index = 0;
	pmap_stacks_decl(pmap_stacks, victim->height);
	pmap_stacks[0] = slot;
	struct pmnode **anchor = &victim->right;
	while (*anchor) {
		pmap_stacks[++index] = anchor;
		anchor = &(*anchor)->left;
	}
	// leftmost node
	struct pmnode *node = *pmap_stacks[index];

	// leftmost_parent->left = leftmost->right; (remove_min_binding)
	(*pmap_stacks[--index])->left = node->right;

	// copy (left, right) to node
	*node = **slot;
	// link the leftmost node to the slot
	*slot = node;
	// updating `&slot->right` after linking. [0] => slot, [1] => &slot->right
	pmap_stacks[1] = &node->right;

	while (index >= 0) {
		pmap_balance(pmap_stacks[index--], &index);
	}
	return victim;
}
