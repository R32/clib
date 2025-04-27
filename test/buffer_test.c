#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "strbuf.h"
#include "wcsbuf.h"
#include "mempool.h"
#include "crlf_counter.h"

struct posnum {
	int pos;
	int num;
};
static int posnum_compare(const struct posnum *a, const struct posnum *b)
{
	return a->pos - b->pos;
}
static void buffer()
{
	const int MAX = 66;
	struct buffer posbuf = { 0 };
	for (int i = 0; i < MAX; i++) {
		struct posnum *data = buffer_incr(&posbuf, sizeof(struct posnum));
		data->pos = i;
		data->num = MAX - i;
	}
	assert(buffer_length(&posbuf) == MAX);
	for (int i = 0; i < MAX; i++) {
		struct posnum *data = buffer_index(&posbuf, sizeof(struct posnum), i);
		assert(data->pos == i && data->num == MAX - i);
		assert(buffer_bsearch(&posbuf, sizeof(struct posnum), data, posnum_compare) == data);
	}
	struct posnum dummy = { MAX , 0 };
	assert(buffer_bsearch(&posbuf, sizeof(struct posnum), &dummy, posnum_compare) == NULL);
	buffer_release(&posbuf);
}

static void mempool()
{
	struct rect { int x, y, w, h;};
	struct mempool pool = (struct mempool){ .size = sizeof(struct rect) };
	const int MAX = 66;
	for (int i = 0; i < MAX; i++) {
		struct rect *rect = mempool_alloc(&pool);
		*rect = (struct rect){ i, -i, MAX - i, MAX + i };
	}
	assert(buffer_length(&pool.inner) == MAX);
	// unsafe index
	for (int i = 0; i < MAX; i += 2) {
		struct rect *rect = buffer_index(&pool.inner, pool.size, i);
		mempool_free(&pool, rect);
	}
	// from freelist
	for (int i = 0; i < MAX; i += 2) {
		struct rect *rect = mempool_alloc(&pool);
		*rect = (struct rect){ i, -i, MAX - i, MAX + i };
	}
	assert(buffer_length(&pool.inner) == MAX);
	mempool_reset(&pool);
	assert(buffer_length(&pool.inner) == 0 && pool.size == sizeof(struct rect));
	assert(pool.inner.head && pool.inner.head == pool.inner.tail);
	mempool_release(&pool);
}

static void crlf_counter()
{
	char text[] =
		"My name is Shakespeare, William\n"
		"I owned a feather quill, I am\n"
		"The writer most familiar to you\n"
		"My way with words amazes me\n"
		"Came up with so many phrases me\n"
		"That still the number dazes me too\n"
		"Oh, \"You've got the be cruel to be kind\"\n"
		"\"If truth were known\", \"Love is blind\"\n"
		"Yet each of these quotes you will find\n"
		"It's what I do\n"
	;
	struct crlf_counter crlf = { 0 };
	// parser
	for (int i = 0; i < sizeof(text); i++) {
		char c = text[i];
		if (c == '\n')
			crlf_add(&crlf, i + 1); // save the next of '\n'
	}
	assert(crlf_length(&crlf) == 1 + 10);
	// first 
	int pos = 0;
	struct line_column lncol = crlf_search(&crlf, pos);
	assert(lncol.line == 1 && lncol.column == 1);
	#define strpos(s) (strstr(text, s) - text)

	pos = strpos("Shakespeare");
	lncol = crlf_search(&crlf, pos);
	assert(lncol.line == 1 && lncol.column == 12);

	pos = strpos("owned");
	lncol = crlf_search(&crlf, pos);
	assert(lncol.line == 2 && lncol.column == 3);

	pos = strpos("amazes");
	lncol = crlf_search(&crlf, pos);
	assert(lncol.line == 4 && lncol.column == 19);

	pos = strpos("quotes");
	lncol = crlf_search(&crlf, pos);
	assert(lncol.line == 9 && lncol.column == 19);

	crlf_reset(&crlf);
	assert(crlf_length(&crlf) == 1);
	lncol = crlf_search(&crlf, 0);
	assert(lncol.line == 1 && lncol.column == 1);

	crlf_release(&crlf);
	assert(crlf_length(&crlf) == 0);
}

static void strbuf()
{
	struct strbuf buf = {0};

	#define TEXT "the quick brown fox jumped over the lazy dog\n"
	for (int i = 0; i < 3; i++) {
		strbuf_append_char(&buf, 'A' + i);
		strbuf_append_int(&buf, 101 + i);
		strbuf_append_char(&buf, '\n');
		strbuf_append_string(&buf, TEXT, strlen(TEXT));
	}
	strbuf_append_double(&buf, 3.1415926535897984626, 17);
	strbuf_append_char(&buf, '\n');
	strbuf_append_float(&buf, 3.1415926535f, 9);
	strbuf_append_char(&buf, '\n');
	strbuf_append_double(&buf, 3.14, 17);
	strbuf_append_char(&buf, '\n');
	strbuf_append_float(&buf, 3.f, 9);
	strbuf_append_char(&buf, '\n');

	char *result = "A101\n" TEXT "B102\n" TEXT "C103\n" TEXT
		"3.1415926535897984\n"
		"3.141592741\n"
		"3.14\n"
		"3.00\n"
		;

	char *ptr = malloc(strbuf_length(&buf) + 1);
	strbuf_to_string(&buf, ptr);
	assert(strlen(ptr) == strlen(result) && strcmp(ptr, result) == 0);
	free(ptr);
	strbuf_reset(&buf);
	assert(buf.inner.head && buf.inner.head == buf.inner.tail && strbuf_length(&buf) == 0);
	strbuf_release(&buf);
	assert(buf.inner.head == NULL);
}

static void wcsbuf()
{
	struct wcsbuf buf = {0};
	#undef TEXT
	#define TEXT L"the quick brown fox jumped over the lazy dog\n"
	for (int i = 0; i < 3; i++) {
		wcsbuf_append_char(&buf, 'A' + i);
		wcsbuf_append_int(&buf, 101 + i);
		wcsbuf_append_char(&buf, '\n');
		wcsbuf_append_string(&buf, TEXT, wcslen(TEXT));
	}
	wcsbuf_append_double(&buf, 3.1415926535897984626, 17);
	wcsbuf_append_char(&buf, '\n');
	wcsbuf_append_float(&buf, 3.1415926535f, 9);
	wcsbuf_append_char(&buf, '\n');
	wcsbuf_append_double(&buf, 3.14, 17);
	wcsbuf_append_char(&buf, '\n');
	wcsbuf_append_float(&buf, 3.f, 9);
	wcsbuf_append_char(&buf, '\n');

#	define HANG_ZI L"ÖÐÎÄºº×Ö\n"
	wcsbuf_append_string(&buf, HANG_ZI, wcslen(HANG_ZI));
	wchar_t *result = L"A101\n" TEXT L"B102\n" TEXT L"C103\n" TEXT
		L"3.1415926535897984\n"
		L"3.141592741\n"
		L"3.14\n"
		L"3.00\n"
		HANG_ZI
		;
	wchar_t *ptr = malloc((wcsbuf_length(&buf) + 1) * sizeof(wchar_t));
	wcsbuf_to_string(&buf, ptr);
	//printf("length: %d\n%ls\n", buf.length, ptr);
	assert(wcslen(ptr) == wcslen(result) && wcscmp(ptr, result) == 0);
	free(ptr);
	wcsbuf_reset(&buf);
	assert(buf.inner.head && buf.inner.head == buf.inner.tail && strbuf_length(&buf) == 0);
	strbuf_release(&buf);
	assert(buf.inner.head == NULL);
}

void buffer_test()
{
	buffer();
	strbuf();
	wcsbuf();
	mempool();
	crlf_counter();
}
