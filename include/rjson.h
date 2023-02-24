/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_JSON_H
#define R_JSON_H

#include "rclibs.h"
#include "ucs2.h"
#include "wcsbuf.h"
#include "tinyalloc.h"
#include "rlex.h"
#include "rstream.h"
#include "rarray.h"
#include "crlf_counter.h"

// lwchars->wcs
typedef wchar_t *rj_wchars;

struct lwchars {
	int len;
	wchar_t wcs[0];
};

#define LWCHARS_OF(rs)                      container_of(((void *)(rs)), struct lwchars, wcs)
#define rj_wchars_length(rs)                (LWCHARS_OF(rs)->len)

enum rjson_kind {
	KNull = 1,
	KBool,
	KNumber,
	KString,
	KObject,
	KArray,
};
// NOTE: It should be created indirectly by "struct rjson_vitem"
struct rjson_value {
	enum rjson_kind                       kind;
	int                                 length; // if KArray
	union {
		int                         istrue; // bool
		double                      number; // number
		rj_wchars                   string; // String
		struct {                            // if KArray or KObject
			struct rjson_vitem   *head;
			struct rjson_vitem   *tail;
		};
	};
};

struct rjson_vitem {
	rj_wchars                              key;
	struct rjson_vitem                   *next;
	struct rjson_value                   value;
};
#define VITEM_OF(v)                          container_of(v, struct rjson_vitem, value)

struct rjson {
	struct wcsbuf                        buffer; // WCHAR
	struct bumpalloc_root               wcspool; // string allocator
	struct fixedalloc_root             nodepool; // node allocator
	struct rjson_value                   *value;
};

struct rjson_parser {
	struct rjson                           json;
	struct rarray                        parray; // <struct pos_wchars>
	struct crlf_counter                 crlfcnt;
	rj_wchars                          filename;
	struct rlex                             lex;
	struct rstream                       stream; // stream
};

struct pos_wchars {
	int pos;
	rj_wchars wcs;
};


C_FUNCTION_BEGIN

void rjson_init(struct rjson *rj);

void rjson_release(struct rjson *rj);

// rj_wchars

rj_wchars rj_wchars_fromwcs(struct rjson *rj, wchar_t *src, int len);

rj_wchars rj_wchars_fromstr(struct rjson *rj, char *src, int len);

rj_wchars rj_wchars_alloc(struct rjson *rj, int len);

rj_wchars rj_wchars_flush(struct rjson *rj, struct wcsbuf *buffer);

// rjson_value

struct rjson_value *rjvalue_null(struct rjson *rj);

struct rjson_value *rjvalue_bool(struct rjson *rj, int istrue);

struct rjson_value *rjvalue_number(struct rjson *rj, double number);

struct rjson_value *rjvalue_from_wcs(struct rjson *rj, wchar_t *wcs, int len);

struct rjson_value *rjvalue_from_cstr(struct rjson *rj, char *str, int len);

struct rjson_value *rjvalue_from_lwchars(struct rjson *rj, struct lwchars *lwcs);

struct rjson_value *rjvalue_object_new(struct rjson *rj);

struct rjson_value *rjvalue_array_new(struct rjson *rj);

struct rjson_value *rjvalue_array_get(struct rjson_value *array, int index);

struct rjson_value *rjvalue_object_get(struct rjson_value *object, wchar_t *key);

// push a value to KArray or KObject
// Adds `child` at the end.
void rjvalue_object_add(struct rjson_value *object, struct rjson_vitem *child);
// Adds `child` at the beginning.
void rjvalue_object_push(struct rjson_value *object, struct rjson_vitem *child);

// object[key] = value
bool rjvalue_object_set(struct rjson *rj, struct rjson_value *object, wchar_t *key, struct rjson_value *value);

void rjvalue_string(struct wcsbuf *buffer, struct rjson_value *value, int tn);

void rjson_print(struct rjson *rj, int tn, FILE *stream);

C_FUNCTION_END
#endif
