#include "ucs2.h"
#include "./utf8_decoder.c"

int utf8towcs(unsigned short *out, const unsigned char *src, int srcbytes)
{
	unsigned int byte;
	unsigned int codep = 0;
	unsigned int state = 0;
	int i = 0;
	const unsigned char *end = srcbytes < 0 ? (const unsigned char *)-1 : src + srcbytes;
	if (out == NULL) {
		while (src < end) {
			byte = *src++;
			decode(&state, &codep, byte);
			if (state == UTF8_ACCEPT) {
				if (codep <= 0xFFFF) {
					i++;
				} else {
					i += 2;
				}
				if (!byte && srcbytes < 0)
					break;
			} else if (state == UTF8_REJECT) {
				break; // ERROR
			}
		}
		return i;
	}
	while (src < end) {
		byte = *src++;
		decode(&state, &codep, byte);
		if (state == UTF8_ACCEPT) {
			if (codep <= 0xFFFF) {
				out[i++] = (unsigned short)codep;
			} else {
				out[i++] = (unsigned short)(0xD7C0 + (codep >> 10));
				out[i++] = (unsigned short)(0xDC00 + (codep & 0x3FF));
			}
			if (!byte && srcbytes < 0)
				break;
		} else if (state == UTF8_REJECT) {
			break; // ERROR
		}
	}
	return i;
}

int wcstoutf8(unsigned char *out, const unsigned short *src, int srcnt)
{
	unsigned int c = 0;
	int i = 0;
	const unsigned short *end = srcnt < 0 ? (const unsigned short *)-1 : src + srcnt;
	if (out == NULL) {
		while (src < end) {
			c = *src++;
			if (c < 0x80) {
				i++;
				if (!c && srcnt < 0)
					break;
			} else if (c < 0x800) {
				i += 2;
			} else if (c >= 0xD800 && c <= 0xDFFF) { // surrogate pair
				if (src == end)
					break;
				i += 4;
			} else {
				i += 3;
			}
		}
		return i;
	}
	while (src < end) {
		c = *src++;
		if (c < 0x80) {
			out[i++] = (unsigned char)c;
			if (!c && srcnt < 0)
				break;
		} else if (c < 0x800) {
			out[i++] = (unsigned char)(0xC0 | (c >> 6));
			out[i++] = (unsigned char)(0x80 | (c & 63));
		} else if (c >= 0xD800 && c <= 0xDFFF) {
			if (src == end)
				break;
			int k = ((((int)c - 0xD800) << 10) | (((int)*src++) - 0xDC00)) + 0x10000;
			out[i++] = (unsigned char)(0xF0 | (k>>18));
			out[i++] = (unsigned char)(0x80 | ((k >> 12) & 63));
			out[i++] = (unsigned char)(0x80 | ((k >> 6) & 63));
			out[i++] = (unsigned char)(0x80 | (k & 63));
		} else {
			out[i++] = (unsigned char)(0xE0 | (c >> 12));
			out[i++] = (unsigned char)(0x80 | ((c >> 6) & 63));
			out[i++] = (unsigned char)(0x80 | (c & 63));
		}
	}
	return i;
}
