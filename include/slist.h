#ifndef R32_SLIST_H
#define R32_SLIST_H
#include "comm.h"

/**
* +------+      +------+       +------+       +------+
* | HEAD |next->|  N3  |next-->|  N2  |next-->|  N1  |next-->NULL
* +------+      +------+       +------+       +------+
*/

#define SLIST_HEAD_INIT {.next = NULL}
#define SLIST_HEAD(name) \
	struct slist_head name = SLIST_HEAD_INIT

#define slist_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define slist_first(head) \
	((head)->next)

#define slist_next(node) \
	((node)->next)

#define slist_first_entry(head, type, member) \
	slist_entry(slist_first(head), type, member)

#define slist_for_each(pos, head) \
	for (pos = slist_first(head); pos; pos = slist_next(pos))

static inline void INIT_SLIST_HEAD(struct slist_head *head) {
	slist_first(head) = NULL;
}
static inline bool slist_empty(struct slist_head* head) {
	return slist_first(head) == NULL;
}
static inline bool slist_singular(struct slist_head* head) {
	return slist_first(head) && slist_first(head)->next == NULL;
}

// Adds node to the beginning of the list if `head` is the HEAD of the list.
// Adds node after `head`. e.g: slist_add(node, tail).
static inline void slist_add(struct slist_head *newz, struct slist_head *head) {
	newz->next = slist_first(head);
	slist_first(head) = newz;
}

// Pops the first item from the HEAD, or NULL if empty.
static inline struct slist_head* slist_pop(struct slist_head *head) {
	struct slist_head* ret = slist_first(head);
	slist_first(head) = ret->next;
	return ret;
}

C_FUNCTION_BEGIN

bool slist_remove(struct slist_head *node, struct slist_head *head);
void slist_rev(struct slist_head *head);
unsigned int slist_len(struct slist_head *head);

C_FUNCTION_END


#endif
