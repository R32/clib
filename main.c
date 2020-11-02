#include <stdio.h>
#include <stdlib.h>
#include "comm.h"
#include "slist.h"
#include "assert.h"

struct blk_s {
	int n;
	struct slist_head link;
};

void t_slist() {
	int len = 10;
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

int main(int argc, char** args) {
	t_slist();
	return 0;
}
