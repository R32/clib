#include "../src/kuai.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

static struct slab *slab_search(struct kuai *kuai, void *block)
{
	struct slab *slab = slab_head(kuai);
	while (slab) {
		if (BPTR(block) > BPTR(slab) && BPTR(block) < BPTR(slab) + slab_pos(slab))
			break;
		slab = slab_next(slab);
	}
	return slab;
}

static int slab_validate(struct kuai *kuai, void *block)
{
	struct slab *slab = slab_search(kuai, block);
	if (!slab)
		return 0;
	if (!slab_isinner(slab))
		return 1;
	return block_validate(slab, block);
}

static int slab_size(struct kuai *kuai, void *block)
{
	struct slab *slab = slab_search(kuai, block);
	if (!slab)
		return 0;
	if (!slab_isinner(slab))
		return slab_pos(slab);
	return block_size(slab, block);
}

static void kuai_print(struct kuai *kuai)
{
	struct slab *slab = slab_head(kuai);
	size_t total = 0;
	size_t fragments = 0;
	int external = 0;
	int count = 0;
	while (slab) {
		external += slab_isinner(slab) ? 0 : 1;
		total += slab_isinner(slab) ? SLAB_SIZE : slab_pos(slab);
		fragments += slab_isinner(slab) ? SLAB_SIZE - slab_pos(slab) : 0;
		slab = slab_next(slab);
		count++;
	}
	printf("slab count : %d, extern : %d, total : %.2fKB, fragments : %.2FKB\n",
		count, external, (double)total / 1024., (double)fragments / 1024.
	);
}

static struct kuai kuai = {0};

#define IS_ALIGNED(ptr)    (((size_t)(ptr) & (BLK_BASE - 1)) == 0)
int onsort(const void* aa, const void* bb)
{
	char* a = *(char**)aa;
	char* b = *(char**)bb;
	assert(IS_ALIGNED(a));
	assert(IS_ALIGNED(b));
	if (a > b) {
		assert(b + slab_size(&kuai, b) <= a);
	} else if (a < b) {
		assert(a + slab_size(&kuai, a) <= b);
	} else {
		assert(0);
	}
	return (int)(a - b);
}

void shuffle(void *a[], int len)
{
	void* tmp = NULL;
	int t;
	for (int i = 0; i < len; i++) {
		t = rand() % len;
		tmp = a[t];
		a[t] = a[i];
		a[i] = tmp;
	}
}

static int pmap_real_height(struct pmnode *node)
{
	if (node == NULL)
		return 0;
	int left = pmap_real_height(node->left);
	int right = pmap_real_height(node->right);
	return 1 + (left >= right ? left : right);
}
#define LAFB_SIZE(m)   ((m)->node.aux)
static void kuai_with_pmap()
{
	struct pmnode *root = NULL;
	assert(lafblock_remove(&root, BLK_BASE) == NULL);

	#define CNT 128
	#define DIV   8
	struct lafblock lafs[CNT];
	for (int i = 0; i < CNT; i++) {
		lafblock_upsert(&root, &lafs[i], (i % DIV) * BLK_BASE);
	}
	assert(pmap_count(root) == DIV);
	// final group
	for (int i = CNT - DIV; i < CNT; i++) {
		int count = 0;
		struct lafblock *ptr = &lafs[i];
		while (ptr) {
			count++;
			ptr = ptr->next ? container_of(ptr->next, struct lafblock, node) : NULL;
		}
		assert(count == CNT / DIV);
	}
	// removing
	for (int i = 0; i < CNT / 2; i++) {
		int size = (i % DIV) * BLK_BASE;
		struct lafblock *blk = lafblock_remove(&root, size); // Exact size
		assert(blk && LAFB_SIZE(blk) == size);
	}
	for (int i = CNT / 2; i < CNT - DIV; i++) {
		int size = (i % DIV) * BLK_BASE;
		struct lafblock *blk = lafblock_remove(&root, size - BLK_BASE / 2); // Approximate size
		assert(blk && LAFB_SIZE(blk) == size);
	}
	// first group
	for (int i = 0; i < DIV; i++) {
		struct lafblock *blk = &lafs[i];
		assert(blk->next == NULL);
		assert(pmap_real_height(&blk->node) == blk->node.height);
	}
	assert(pmap_count(root) == DIV);
	// clear final group
	for (int i = CNT - DIV; i < CNT; i++) {
		int size = (i % DIV) * BLK_BASE;
		struct lafblock *blk = lafblock_remove(&root, size);
		assert(blk && LAFB_SIZE(blk) == size);
	}
	assert(root == NULL);
}

static void kuai_test_inner(int log)
{
#define kt_alloc(size)     kuai_alloc(&kuai, size)
#define kt_free(ptr)       kuai_free(&kuai, ptr)
#define kt_validate(b)     slab_validate(&kuai, b)
#define kt_size(b)         slab_size(&kuai, b)
	// test without kuai_init
	{
	void *external = kt_alloc(EXTERN_SIZE + BLK_BASE); // external allocating
	assert(slab_size(&kuai, external) == EXTERN_SIZE + BLK_BASE + STAT_SIZE);
	struct slab *slab = slab_head(&kuai);
	assert(slab && slab_isinner(slab) == 0);
	kuai_reset(&kuai);
	assert(slab_head(&kuai) == NULL);
	}
	kuai_destroy(&kuai);
	// kuai_init(&kuai);
	// fixed alloc
	for (int num = 1; num < KFREELIST_MAX; num++) {
		int size = BLK_BASE * num;
		int count = (SLAB_SIZE - BMPBYTE_SIZE) / size;
		for (int i = 0; i < count; i++) {
			uint64_t *block = kt_alloc(size);
			assert(kt_validate(block));
			assert(kt_size(block) == size);
			// fill with 0xCC
			int j = 0;
			int len = size / sizeof(uint64_t);
			while (j < len) {
				block[j++] = 0xCCCCCCCCCCCCCCCC;
			}
		}
		assert(slab_next(kuai.slab) == NULL);
		assert(slab_pos(kuai.slab) == BMPBYTE_SIZE + (count * size));
		unsigned char *first = kuai.slab->data;
		for (int i = 0; i < count; i++) {
			uint64_t *block = (uint64_t *)(first + i * size);
			assert(kt_validate(block));
			int j = 0;
			int len = size / sizeof(uint64_t);
			while (j < len) {
				assert(block[j++] == 0xCCCCCCCCCCCCCCCC);
			}
			kt_free(block);
			assert(FREE_HEAD(&kuai, free_index(size)) == block);
		}
		// reset
		kuai_reset(&kuai);
		assert(slab_pos(kuai.slab) = BMPBYTE_SIZE);
		assert(slab_committed(kuai.slab) = COMMIT_BASE);
	}
// rands
#define RAND()        (rand() % (EXTERN_SIZE + 128))
#define COUNT         (1080)
#define FL_MAXSIZE    (KFREELIST_MAX * BLK_BASE)
	void *list[COUNT];
	// new alloc
	for (int j = 0; j < 10; j++) {
		kuai_reset(&kuai);
		for (int i = 0; i < COUNT; i++) {
			int size = ALIGN_UP(1 + RAND(), BLK_BASE);
			list[i] = kt_alloc(size);
			if (size <= EXTERN_SIZE) {
				assert(kt_size(list[i]) == size);
			} else {
				assert(kt_size(list[i]) == size + STAT_SIZE);
			}
		}
		shuffle((void **)list, COUNT);
		qsort(list, COUNT, sizeof(list[0]), onsort);
		shuffle((void **)list, COUNT);
	}
	// free halfcount
	for (int i = 0; i < COUNT / 2; i++)
		kt_free(list[i]);
	// realloc halfcount
	for (int i = 0; i < COUNT / 2; i++) {
		int size = ALIGN_UP(1 + RAND(), BLK_BASE);
		list[i] = kt_alloc(size);
		int real = kt_size(list[i]);
		if (size > EXTERN_SIZE) {
			assert(real == size + STAT_SIZE);
			continue;
		}
		if (size < FL_MAXSIZE) {
			assert(real == size);
		} else {
			assert(real >= size && real < size + FL_MAXSIZE);
		}
	}
	shuffle((void **)list, COUNT);
	qsort(list, COUNT, sizeof(list[0]), onsort);

	// free halfcount
	for (int i = COUNT / 2; i < COUNT; i++)
		kt_free(list[i]);
	// realloc halfcount
	for (int i = COUNT / 2; i < COUNT; i++) {
		int size = ALIGN_UP(RAND() + 1, BLK_BASE);
		list[i] = kt_alloc(size);
		int real = kt_size(list[i]);
		if (size > EXTERN_SIZE) {
			assert(real == size + STAT_SIZE);
			continue;
		}
		if (size < FL_MAXSIZE) {
			assert(real == size);
		} else {
			assert(real >= size && real < size + FL_MAXSIZE);
		}
	}
	shuffle((void **)list, COUNT);
	qsort(list, COUNT, sizeof(list[0]), onsort);
	if (log) kuai_print(&kuai);
	kuai_destroy(&kuai);
}

void kuai_test(int n)
{
	srand((uint32_t)time(NULL));
	kuai_with_pmap();
	for (int i = 0; i < n; i++) {
		kuai_test_inner(0);
	}
}
