/*
 * SPDX-License-Identifier: GPL-2.0
 */
#include "rjson.h"

#define rj_lenwcs_new(rj, n)   bumpalloc(&rj->wcspool, (n) * sizeof(wchar_t))
#define rj_vitem_new(rj)       fixedalloc(&rj->nodepool)

rj_wchars rj_wchars_fromwcs(struct rjson *rj, wchar_t *src, int len)
{
	if (len < 0) {
		len = wcslen(src); // without '\0'
	}
	struct lwchars *lwcs = rj_lenwcs_new(rj, len + (1 + sizeof(int) / sizeof(wchar_t)));
	lwcs->len = len;
	wmemcpy(lwcs->wcs, src, len);
	lwcs->wcs[len] = 0;
	return lwcs->wcs;
}

rj_wchars rj_wchars_fromstr(struct rjson *rj, char *src, int len)
{
	if (len < 0)
		len = utf8towcs(NULL, src, -1) - 1; // without '\0'
	struct lwchars *lwcs = rj_lenwcs_new(rj, len + (1 + sizeof(int) / sizeof(wchar_t)));
	lwcs->len = len;
	utf8towcs(lwcs->wcs, src, len);
	lwcs->wcs[len] = 0;
	return lwcs->wcs;
}

rj_wchars rj_wchars_alloc(struct rjson *rj, int len)
{
	struct lwchars *lwcs = rj_lenwcs_new(rj, len + (1 + sizeof(int) / sizeof(wchar_t)));
	lwcs->len = len;
	lwcs->wcs[len] = 0;
	return lwcs->wcs;
}

int rj_wchars_length(rj_wchars wcs)
{
	return container_of(wcs, struct lwchars, wcs)->len;
}

void rjson_init(struct rjson *rj)
{
	*rj = (struct rjson){0}; // memset(rj, 0, sizeof(struct rjson));
	rj->buffer.csize = 128;
	rj->wcspool.chksize = 4;
	rj->nodepool.chksize = 4;
	rj->nodepool.size = sizeof(struct rjson_vitem);
}

void rjson_release(struct rjson *rj)
{
	wcsbuf_release(&rj->buffer);
	bumpdestroy(&rj->wcspool);
	fixeddestroy(&rj->nodepool);
}

struct rjson_value *rjvalue_null(struct rjson *rj)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KNull;
	return &vitem->value;
}

struct rjson_value *rjvalue_bool(struct rjson *rj, int istrue)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KBool;
	vitem->value.istrue = istrue;
	return &vitem->value;
}

struct rjson_value *rjvalue_number(struct rjson *rj, double number)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KNumber;
	vitem->value.number = number;
	return &vitem->value;
}

struct rjson_value *rjvalue_object_new(struct rjson *rj)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	*vitem = (struct rjson_vitem){0};
	vitem->value.kind = KObject;
	return &vitem->value;
}

struct rjson_value *rjvalue_array_new(struct rjson *rj)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	*vitem = (struct rjson_vitem){0};
	vitem->value.kind = KArray;
	return &vitem->value;
}

struct rjson_value *rjvalue_from_wcs(struct rjson *rj, wchar_t *wcs, int len)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KString;
	vitem->value.rjwcs = rj_wchars_fromwcs(rj, wcs, len);
	return &vitem->value;
}

struct rjson_value *rjvalue_from_cstr(struct rjson *rj, char *str, int len)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KString;
	vitem->value.rjwcs = rj_wchars_fromstr(rj, str, len);
	return &vitem->value;
}

struct rjson_value *rjvalue_from_lwchars(struct rjson *rj, struct lwchars *lwcs)
{
	struct rjson_vitem *vitem = rj_vitem_new(rj);
	vitem->value.kind = KString;
	vitem->value.rjwcs = lwcs->wcs;
	return &vitem->value;
}

// object->kind could be KObject or KArray
void rjvalue_object_add(struct rjson_value *object, struct rjson_vitem *child)
{
	if (object->kind < KObject)
		return;
	child->next = NULL;
	if (object->head == NULL)
		object->head = child;
	else
		object->tail->next = child;
	object->tail = child;
	object->length++;
}

#define rjvalue_first(obj)  ((obj)->head)
#define is_ended(k)         (!(k)[0])

struct rjson_value *rjvalue_array_get(struct rjson_value *array, int index)
{
	if (array->kind < KObject)
		return NULL;
	struct rjson_vitem *vitem = rjvalue_first(array);
	int i = 0;
	while (vitem) {
		if (i++ == index)
			return &vitem->value;
		vitem = vitem->next;
	}
	return NULL;
}

struct rjson_value *rjvalue_object_get(struct rjson_value *object, wchar_t *key)
{
	if (key == NULL || is_ended(key))
		return object;
	int len;
	wchar_t *dot = NULL;
	struct rjson_vitem *child;
	while (object && object->kind == KObject) {
		dot= wcschr(key, '.');
		if (dot == NULL)
			dot = key + wcslen(key);
		len = (int)(dot - key);
		child = rjvalue_first(object);
		while (child && wcsncmp(child->key, key, len)) {
			child = child->next;
		}
		object = child ? &child->value : NULL;
		if (is_ended(dot))
			return object;
		key = dot + 1;
	}
	return NULL;
}

bool rjvalue_object_set(struct rjson *rj, struct rjson_value *object, wchar_t *key, struct rjson_value *value)
{
	if (!rj)
		return false;
	if (!object) {
		object = rj->value;
		if (!object) {
			object = rjvalue_object_new(rj);
			rj->value = object;
		}
	}
	int len;
	wchar_t *dot;
	struct rjson_vitem *child;
	while (object && object->kind == KObject) {
		dot= wcschr(key, '.');
		if (dot == NULL)
			dot = key + wcslen(key);
		len = (int)(dot - key);
		child = rjvalue_first(object);
		while (child && wcsncmp(child->key, key, len)) {
			child = child->next;
		}
		if (is_ended(dot)) {
			if (child) { // if found
				child->value = *value;
			} else {
				child = VITEM_OF(value);
				child->key = rj_wchars_fromwcs(rj, key, len);
				rjvalue_object_add(object, child);
			}
			return true;
		}
		if (child == NULL) {
			child = VITEM_OF(rjvalue_object_new(rj));
			child->key = rj_wchars_fromwcs(rj, key, len);
			rjvalue_object_add(object, child);
		}
		object = &child->value;
		key = dot + 1;
	}
	return false;
}

const static wchar_t *wcstabs[] = {
	L"",
	L"\t",
	L"\t\t",
	L"\t\t\t",
	L"\t\t\t\t",
	L"\t\t\t\t\t",
	L"\t\t\t\t\t\t",
	L"\t\t\t\t\t\t\t",
	L"\t\t\t\t\t\t\t\t",
	L"\t\t\t\t\t\t\t\t\t",
};

#define ADD_STRING(ws, len)    wcsbuf_append_string(buffer, ws, len)
#define ADD_CHAR(c)            wcsbuf_append_char(buffer, c)
#define ADD_NUMBER(n)          wcsbuf_append_double(buffer, n, -1)

static void add_with_unescape(struct wcsbuf *buffer, wchar_t *wcs, int len)
{
	wchar_t *max = wcs + len;
	int c;
	while (wcs < max) {
		c = *wcs++;
		switch(c) {
		case '"':
			ADD_CHAR('\\');
			ADD_CHAR('"');
			break;
		case '\n':
			ADD_CHAR('\\');
			ADD_CHAR('n');
			break;
		case '\r':
			ADD_CHAR('\\');
			ADD_CHAR('r');
			break;
		default:
			ADD_CHAR(c);
		}
	}
}

static void output_compact(struct wcsbuf *buffer, struct rjson_value *value)
{
	struct rjson_vitem *child;
	switch(value->kind) {
	case KNull:
		ADD_STRING(L"null", 4);
		break;
	case KBool:
		if (value->istrue)
			ADD_STRING(L"true", 4);
		else
			ADD_STRING(L"false", 5);
		break;
	case KNumber:
		ADD_NUMBER(value->number);
		break;
	case KString:
		ADD_CHAR('"');
		add_with_unescape(buffer, value->rjwcs, rj_wchars_length(value->rjwcs));
		ADD_CHAR('"');
		break;
	case KObject:
		ADD_CHAR('{');
		child = rjvalue_first(value);
		while (child) {
			ADD_CHAR('"');
			add_with_unescape(buffer, child->key, rj_wchars_length(child->key));
			ADD_CHAR('"');
			ADD_CHAR(':');
			output_compact(buffer, &child->value);
			child = child->next;
			if (child)
				ADD_CHAR(',');
		}
		ADD_CHAR('}');
		break;
	case KArray:
		ADD_CHAR('[');
		child = rjvalue_first(value);
		while (child) {
			output_compact(buffer, &child->value);
			child = child->next;
			if (child)
				ADD_CHAR(',');
		}
		ADD_CHAR(']');
		break;
	default:
		break;
	}
}

static void output_normal(struct wcsbuf *buffer, struct rjson_value *value, int tn)
{
	struct rjson_vitem *child;
	if (tn >= (ARRAYSIZE(wcstabs) - 1))
		tn = ARRAYSIZE(wcstabs) - 2;
	switch(value->kind) {
	case KNull:
		ADD_STRING(L"null", 4);
		break;
	case KBool:
		if (value->istrue)
			ADD_STRING(L"true", 4);
		else
			ADD_STRING(L"false", 5);
		break;
	case KNumber:
		ADD_NUMBER(value->number);
		break;
	case KString:
		ADD_CHAR('"');
		add_with_unescape(buffer, value->rjwcs, rj_wchars_length(value->rjwcs));
		ADD_CHAR('"');
		break;
	case KObject:
		ADD_CHAR('{');
		child = rjvalue_first(value);
		while (child) {
			ADD_CHAR('\n');
			ADD_STRING((wchar_t *)wcstabs[tn + 1], tn + 1);
			ADD_CHAR('"');
			add_with_unescape(buffer, child->key, rj_wchars_length(child->key));
			ADD_CHAR('"');
			ADD_CHAR(' '); ADD_CHAR(':'); ADD_CHAR(' ');
			output_normal(buffer, &child->value, tn + 1);
			child = child->next;
			if (child) {
				ADD_CHAR(',');
			} else {
				ADD_CHAR('\n');
				ADD_STRING((wchar_t *)wcstabs[tn], tn);
			}
		}
		ADD_CHAR('}');
		break;
	case KArray:
		ADD_CHAR('[');
		child = rjvalue_first(value);
		while (child) {
			ADD_CHAR('\n');
			ADD_STRING((wchar_t *)wcstabs[tn + 1], tn + 1);

			output_normal(buffer, &child->value, tn + 1);

			child = child->next;
			if (child) {
				ADD_CHAR(',');
			} else {
				ADD_CHAR('\n');
				ADD_STRING((wchar_t *)wcstabs[tn], tn);
			}
		}
		ADD_CHAR(']');
		break;
	default:
		break;
	}
}

void rjvalue_string(struct wcsbuf *buffer, struct rjson_value *value, int tn)
{
	if (!value)
		return;
	if (tn < 0)
		output_compact(buffer, value);
	else
		output_normal(buffer, value, tn);
	ADD_CHAR('\n');
}

void rjson_print(struct rjson *rj, int tn, FILE *stream)
{
	wcsbuf_reset(&rj->buffer);
	rjvalue_string(&rj->buffer, rj->value, tn);
	wcsbuf_to_file_utf8(&rj->buffer, stream);
}
