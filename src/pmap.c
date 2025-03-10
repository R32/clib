/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 * Copyright (C) 2025 LiuWM
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

#include "pmap.h"

static int inline pmap_height(struct pmnode *node)
{
	return node ? node->height : 0;
}

static int inline imax(int a, int b)
{
	return a > b ? a : b;
}

struct pmnode *pmap_balance(struct pmnode **slot)
{
	struct pmnode *node = *slot;
	struct pmnode **rope = node->rope;
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
			left->right = node;
			node->height = imax(hLR, hR) + 1;
			left->height = imax(hLL, node->height) + 1;
			node = left;
		} else {
			/*      N              LR
			 *    L   R        L        N
			 *  LL LR       (LL LRL) (LRR R)
			 *   (LRL LRR)
			 */
			struct pmnode *LR = left->right;
			left->right = LR->left;
			node->left = LR->right;
			left->height = imax(hLL, pmap_height(LR->left)) + 1;
			node->height = imax(pmap_height(LR->right), hR) + 1;

			LR->left = left;
			LR->right = node;
			LR->height = imax(left->height, node->height) + 1;
			node = LR;
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
			 right->left = node;
			 node->height = imax(hL, hRL) + 1;
			 right->height = imax(node->height, hRR) + 1;
			 node = right;
		} else {
			/*     N                RL
			 *   L   R          N        R
			 *    (RL RR)    (L RLL) (RLR RR)
			 * (RLL RLR)
			 */
			struct pmnode *RL = right->left;
			node->right = RL->left;
			right->left = RL->right;

			node->height = imax(hL, pmap_height(RL->left)) + 1;
			right->height = imax(pmap_height(RL->right), hRR) + 1;

			RL->left = node;
			RL->right = right;
			RL->height = imax(node->height, right->height) + 1;
			node = RL;
		}
	} else {
		int height = imax(hL, hR) + 1;
		if (height == node->height) {
			node->rope = NULL;
		} else {
			node->height = height;
		}
		return node;
	}
	// linking
	node->rope = rope;
	*slot = node;
	return node;
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
	struct pmnode **anchor = &(*slot)->right;
	struct pmnode **leftmost = NULL;
	while (*anchor) {
		(*anchor)->rope = leftmost;
		leftmost = anchor;
		anchor = &(*anchor)->left;
	}
	struct pmnode *node = *leftmost;

	// start balancing at leftmost_parent
	leftmost = node->rope;

	// leftmost_parent->left = leftmost->right; (remove_min_binding)
	(*leftmost)->left = node->right;

	while (leftmost) {
		leftmost = pmap_balance(leftmost)->rope;
	}
	// link the leftmost node to the slot
	(*slot)->rope = NULL;
	*node = **slot;
	*slot = node;
	pmap_balance(slot);
	return victim;
}
