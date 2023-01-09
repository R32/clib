#include "ucs2.h"
// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = {
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
	12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};

static uint32_t inline decode(uint32_t *state, uint32_t *codep, uint32_t byte)
{
	uint32_t type = utf8d[byte];

	*codep = (*state != UTF8_ACCEPT) ?
	  (byte & 0x3fu) | (*codep << 6) :
	  (0xff >> type) & (byte);

	*state = utf8d[256 + *state + type];
	return *state;
}

/*
* SPDX-License-Identifier: GPL-2.0
*/
int utf8towcs(wchar_t *out, const unsigned char *src)
{
	uint32_t byte;
	uint32_t codep = 0;
	uint32_t state = 0;
	int i = 0;
	if (out == NULL) {
		while (byte = *src++) {
			decode(&state, &codep, byte);
			if (state == UTF8_REJECT) {
				return -1; // ERROR
			} else if (state == UTF8_ACCEPT) {
				if (codep < 0xFFFF) {
					++ i;
				} else {
					i += 2;
				}
			}
		}
	} else {
		while (byte = *src++) {
			decode(&state, &codep, byte);
			if (state == UTF8_REJECT) {
				return -1; // ERROR
			} else if (state == UTF8_ACCEPT) {
				if (codep < 0xFFFF) {
					out[i++] = (wchar_t)codep;
				} else {
					out[i++] = (wchar_t) (0xD7C0 + (codep >> 10));
					out[i++] = (wchar_t) (0xDC00 + (codep & 0x3FF));
				}
			}
		}
	}
	return i;
}

int wcstoutf8(unsigned char *out, const wchar_t *src)
{
	wchar_t c = 0;
	int i = 0;
	if (out == NULL) {
		while (c = *src++) {
			if (c < 0x80) {
				i++;
			} else if (c < 0x800) {
				i += 2;
			} else if (c >= 0xD800 && c <= 0xDFFF) { // surrogate pair
				src ++;
				i += 4;
			} else {
				i += 3;
			}
		}
	} else {
		while (c = *src++) {
			if (c < 0x80) {
				out[i++] = (unsigned char)c;
			} else if (c < 0x800) {
				out[i++] = (unsigned char)(0xC0 | (c >> 6));
				out[i++] = (unsigned char)(0x80 | (c & 63));
			} else if (c >= 0xD800 && c <= 0xDFFF) {
				int k = ((((int)c - 0xD800) << 10) | (((int)*src++) - 0xDC00)) + 0x10000;
				out[i++] = (unsigned char)(0xF0 |(k>>18));
				out[i++] = (unsigned char)(0x80 | ((k >> 12) & 63));
				out[i++] = (unsigned char)(0x80 | ((k >> 6) & 63));
				out[i++] = (unsigned char)(0x80 | (k & 63));
			} else {
				out[i++] = (unsigned char)(0xE0 | (c >> 12));
				out[i++] = (unsigned char)(0x80 | ((c >> 6) & 63));
				out[i++] = (unsigned char)(0x80 | (c & 63));
			}
		}
	}
	return i;
}
