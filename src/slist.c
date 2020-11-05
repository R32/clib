#include "slist.h"

bool slist_remove(struct slist_head *node, struct slist_head *head) {
	struct slist_head *prev = head;
	struct slist_head *curr = slist_first(head);
	while(curr) {
		if (curr == node) {
			prev->next = curr->next;
			return true;
		}
		prev = curr;
		curr = curr->next;
	}
	return false;
}

void slist_rev(struct slist_head *head) {
	struct slist_head *nxt = NULL;
	struct slist_head *cur = slist_first(head);
	INIT_SLIST_HEAD(head);
	while(cur) {
		nxt = cur->next;
		slist_add(cur, head);
		cur = nxt;
	}
}

unsigned int slist_len(struct slist_head *head) {
	unsigned int i = 0;
	struct slist_head* pop;
	slist_for_each(pop, head) {
		i++;
	}
	return i;
}
