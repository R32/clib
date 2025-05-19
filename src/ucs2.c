/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#include <ctype.h>
#include <limits.h>
#include "ucs2.h"

#ifndef UCHAR_NATIVE_FUN

int ucslen(const uchar *ucs)
{
	const uchar *ptr = ucs;
	while (*ptr)
		ptr++;
	return (int)(ptr - ucs);
}

uchar *ucscpy(uchar *dst, const uchar *src)
{
	uchar *ret = dst;
	while ((*dst++ = *src++));
	return ret;
}

uchar *ucsncpy(uchar *dst, const uchar *src, unsigned int n)
{
	uchar* ret = dst;
	while (n-- && (*dst++ = *src++));
	return ret;
}

int ucscmp(const uchar *s1, const uchar *s2)
{
	int d;
	while ((d = *s1 - *s2++) == 0 && *s1++);
	return d;
}

int ucsncmp(const uchar *s1, const uchar *s2, unsigned int n)
{
	int d = 0;
	while (n-- && (d = *s1 - *s2++) == 0 && *s1++);
	return d;
}

uchar *ucscat(uchar *dst, const uchar *src)
{
	uchar *ptr = dst;
	while (*ptr)
		ptr++;
	while ((*ptr++ = *src++));
	return dst;
}

uchar *ucsncat(uchar *dst, const uchar *src, unsigned int n)
{
	uchar *ptr = dst;
	while (*ptr)
		ptr++;
	while (n-- && (*ptr++ = *src++));
	if ((signed int)n < 0)
		*ptr = 0;
	return dst;
}

uchar *ucschr(const uchar* ucs, uchar ch)
{
	while (*ucs && *ucs != ch)
		ucs++;
	return *ucs == ch ? (uchar *)ucs : NULL;
}

uchar *ucsrchr(const uchar* ucs, uchar ch)
{
	const uchar *start = ucs;

	while (*ucs++);

	while (--ucs != start && *ucs != ch);

	return *ucs == ch ? (uchar *)ucs : NULL;
}

uchar *ucsstr(const uchar *ucs, const uchar *sub)
{
	uchar *cp = (uchar *)ucs;
	uchar *s1, *s2;
	if (!*sub)
		return cp;
	while (*cp) {
		s1 = cp;
		s2 = (uchar *)sub;
		while (*s1 && *s2 && !(*s1 - *s2)) {
			s1++;
			s2++;
		}
		if (!*s2)
			return cp;
		cp++;
	}
	return NULL;
}

#define lower(ch)  ((ch) | 0x20)

double ucstod(const uchar *ucs, uchar **end)
{
	const uchar *head = ucs;
	// skips
	int ch = *head++;
	while (isspace(ch))
		ch = *head++;

	int neg = ch == '-';
	if (neg || ch == '+')
		ch = *head++;

	const uchar *ptr = head;

	double value = 0.;
	while (isdigit(ch)) {
		value = value * 10. + (ch - '0');
		ch = *ptr++;
	}
	if (ch == '.') {
		head += ptr == head;
		ch = *ptr++;
		double fract = 1.;
		while (isdigit(ch)) {
			fract *= 0.1;
			value += fract * (ch - '0');
			ch = *ptr++;
		}
	}
	#define NOT_EMPTY()  (ptr > head)
	#define NO_DOT()     (ucs == head || head[-2] != '.')
	if (!isalpha(ch))
		goto exit;
	ch = lower(ch);
	if (NOT_EMPTY() && ch == 'e') {
		ch = *ptr++;
		int exp = 0;
		double base = ch == '-' ? 0.1 : 10.;
		if (ch == '-' || ch == '+')
			ch = *ptr++;

		while (isdigit(ch)) {
			exp = 10 * exp + (ch - '0');
			ch = *ptr++;
		}
		while (exp) {
			if (exp & 1)
				value *= base;
			exp >>= 1;
			base *= base;
		}
	} else if (NOT_EMPTY() && (ch == 'l' || ch == 'f')) {
		ptr++;
	} else if (ch == 'n' && lower(head[0]) == 'a' && lower(head[1]) == 'n' && NO_DOT()) {
		value = -(1e300 * 1e300 * 0.0);
		ptr += 3;
	} else if (ch == 'i' && lower(head[0]) == 'n' && lower(head[1]) == 'f' && NO_DOT()) {
		value = 1e300 * 1e300;
		ptr += 3;
	}
exit:
	if (end)
		*end = (uchar *)(NOT_EMPTY() ? ptr - 1 : ucs);
	return neg ? -value : value;
#undef NOT_EMPTY
#undef NO_DOT
}

/*
 * NOTE : If overflows, the result is unspecified.
 */
long ucstol(const uchar *ucs, uchar **end, int base)
{
	const uchar *head = ucs;

	// skips
	int ch = *head++;
	while (isspace(ch))
		ch = *head++;

	int neg = ch == '-';
	if (neg || ch == '+')
		ch = *head++;

	const uchar *ptr = head;
	#define NOT_EMPTY() (ptr > head)
	long dig = 0;
	if (base == 0) { // auto base
		if (ch == '0' && (*ptr == 'x' || *ptr == 'X')) {
			base = 16;
			ptr++;
			ch = *ptr++;
		} else {
			base = 10;
		}
	} else if (base < 2 || base > 36) { // bad base
		goto exit;
	}
	if (base <= 10) {
		while (isdigit(ch)) {
			dig = dig * base - (ch - '0'); // Compute dig as a negative number
			ch = *ptr++;
		}
	} else {
		int x;
		while (1) {
			if (isdigit(ch)) {
				x = ch - '0';
			} else if (isalpha(ch)) {
				x = lower(ch) - 'a' + 10;
				if (x > base)
					break;
			} else {
				break;
			}
			dig = dig * base - x;
			ch = *ptr++;
		}
	}
	// Following MSVC's wcstol, which does not process the suffix.
	// if (NOT_EMPTY() && isalpha(ch)) {
	// 	ch = lower(ch);
	// 	if (ch == 'u') {
	// 		ptr += 1 + (lower(ptr[0]) == 'l');
	// 	} else if (ch == 'l') {
	// 		ptr++;
	// 	}
	// }
exit:
	if (end)
		*end = (uchar *)(NOT_EMPTY() ? ptr - 1 : ucs);
	return neg == 0 ? -dig : dig;
	#undef NOT_EMPTY
}

#endif

static inline void ureverse(uchar *i, uchar *j)
{
	uchar t;
	while (i < j) {
		 t   = *j; 
		*j-- = *i; 
		*i++ =  t;
	}
}

int itoua(int value, uchar *out)
{
	unsigned int num = (unsigned int)(value < 0 ? -value : value);
	uchar *ptr = out;
	do { *ptr++ = (uchar)(num % 10 + '0'); } while (num /= 10);
	if (value < 0) 
		*ptr++ = '-';
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}
int uitoua(unsigned int num, uchar *out)
{
	uchar *ptr = out;
	do { *ptr++ = (uchar)(num % 10 + '0'); } while (num /= 10);
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}
int uitoua16(unsigned int num, uchar *out)
{
	unsigned int dig;
	uchar *ptr = out;
	do {
		dig = (unsigned int)(num % 16);
		*ptr++ = (uchar)(dig < 10 ? dig + '0' : dig - 10 + 'a');
	} while (num /= 16);
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}
int i64toua(long long value, uchar *out)
{
	unsigned long long num = (unsigned long long)(value < 0 ? -value : value);
	uchar *ptr = out;
	do { *ptr++ = (uchar)(num % 10 + '0'); } while (num /= 10);
	if (value < 0)
		*ptr++ = '-';
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}
int u64toua(unsigned long long num, uchar *out)
{
	uchar *ptr = out;
	do { *ptr++ = (uchar)(num % 10 + '0'); } while (num /= 10);
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}
int u64toua16(unsigned long long num, uchar *out)
{
	unsigned int dig;
	uchar *ptr = out;
	do {
		dig = (unsigned int)(num % 16);
		*ptr++ = (uchar)(dig < 10 ? dig + '0' : dig - 10 + 'a');
	} while (num /= 16);
	*ptr = 0;
	ureverse(out, ptr - 1);
	return ptr - out;
}

// input "9.999" => "10.000"
static int flt_carryup(uchar *begin, uchar *end)
{
	uchar *ptr = end;
	int ch;
	while (end >= begin) {
		ch = *end;
		if (ch >= '0' && ch < '9') {
			*end = ch + 1;
			return 0;
		}
		if (ch == '.') {
			end--;
			continue;
		}
		if (ch == '-') {
			begin++;
			break;
		}
		*end-- = '0';
	}
	// memove
	while (ptr >= begin) {
		ptr[1] = ptr[0]; // Some compiler will make wrong code for : "*(ptr + 1) = *ptr--"
		ptr--;
	}
	*begin = '1';
	return 1;
}

#define IS_NANF(f)  (((*(unsigned int *)&f) & 0x7fffffff)  > 0x7f800000)
#define IS_INFF(f)  (((*(unsigned int *)&f) & 0x7fffffff) == 0x7f800000)

/*
 * Converts a float to a uchar string.
 *
 * @param value
 * @param out : Pointer to a buffer large enough to hold the result.(including NULL terminator)
 * @param precision : If -1, defaults to 8. If 'fixed' is set, then this value will be ignored.
 * @param fixed : Number of fractional digits to retain.
 * @return : Count of the uchar Characters to out. (not including NULL terminator)
 */
int ftoua(float value, uchar *out, int precision, int fixed)
{
	uchar *ptr = out;
	if (IS_NANF(value)) {
		*ptr++ = 'n';
		*ptr++ = 'a';
		*ptr++ = 'n';
		goto exit;
	}
	if (value < 0) {
		*ptr++ = '-';
		value = -value;
	}
	if (IS_INFF(value)) {
		*ptr++ = 'i';
		*ptr++ = 'n';
		*ptr++ = 'f';
		goto exit;
	}
	// int part
	int dig = 0;
	int len = 0;
	if (value >= 1.0f) {
		dig = (int)value;
		len = uitoua(dig, ptr);
		ptr += len;
	} else {
		*ptr++ = '0';
		if (fixed <= 0) {
			// The first non-zero digit for "precision"
			float x = 0.1f;
			while (value < x) {
				len--;
				x *= 0.1f;
			}
		}
	}
	// precision & fixed
	if (fixed > 0) {
		precision = fixed;
	} else if (!precision) {
		goto exit;
	} else {
		if (precision < 0)
			precision = 8;
		precision -= len;
	}
	// fractional part
	*ptr++ = '.';
	uchar *pdot = ptr;
	float fract = value - dig;
	for (int i = 0; i < precision; i++) {
		fract *= 10.f;
		dig = (int)fract;
		fract -= dig;
		*ptr++ = dig + '0';
	}
	// round up
	if (fract > 0.5f && precision > 0)
		ptr += flt_carryup(out, ptr - 1);

	// strip zeros
	while (ptr[-1] == '0' && ptr - pdot > fixed)
		ptr--;
	if (ptr[-1] == '.')
		ptr--;
exit:
	*ptr = 0;
	return ptr - out;
}

#if defined(__GNUC__) || defined(__clang__)
#   define IS_NAN(d)   __builtin_isnan(d)
#   define IS_INF(d)   __builtin_isinf(d)
#else
#   define IS_NAN(d)   !(d == d)
#   define IS_INF(x)   (((*(unsigned long long *)&(x)) & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL)
#endif

/*
 * Converts a double to a uchar string.
 * 
 * @param value
 * @param out : Pointer to a buffer large enough to hold the result.(including NULL terminator)
 * @param precision : If -1, defaults to 16. If 'fixed' is set, then this value will be ignored.
 * @param fixed : Number of fractional digits to retain.
 * @return : Count of the uchar Characters to out. (not including NULL terminator)
 */
int dtoua(double value, uchar *out, int precision, int fixed)
{
	uchar *ptr = out;
	if (IS_NAN(value)) {
		*ptr++ = 'n';
		*ptr++ = 'a';
		*ptr++ = 'n';
		goto exit;
	}
	if (value < 0) {
		*ptr++ = '-';
		value = -value;
	}
	if (IS_INF(value)) {
		*ptr++ = 'i';
		*ptr++ = 'n';
		*ptr++ = 'f';
		goto exit;
	}
	unsigned long long intpart = 0; 
	int len = 0;
	if (value >= 1.0) {
		intpart = (unsigned long long)value;
		len = u64toua(intpart, ptr);
		ptr += len;
	} else {
		*ptr++ = '0';
		if (fixed <= 0) { 
			// The first non-zero digit for "precision"
			double x = 0.1;
			while (value < x) {
				len--;
				x *= 0.1;
			}
		}
	}
	// precision & fixed
	if (fixed > 0) {
		precision = fixed;
	} else if (!precision) {
		goto exit;
	} else {
		if (precision < 0)
			precision = 16;
		precision -= len;
	}
	// fractional part
	int dig;
	*ptr++ = '.';
	uchar *pdot = ptr;
	double fract = value - intpart;
	for (int i = 0; i < precision; i++) {
		fract *= 10.;
		dig = (int)fract;
		fract -= dig;
		*ptr++ = dig + '0';
	}
	// round up
	if (fract > 0.5 && precision > 0)
		ptr += flt_carryup(out, ptr - 1);
	// strip zeros
	while (ptr[-1] == '0' && ptr - pdot > fixed)
		ptr--;
	if (ptr[-1] == '.')
		ptr--;
exit:
	*ptr = 0;
	return ptr - out;
}

/*
 * Convert uchar string to utf-8.
 * 
 * @param max : Maximum number of bytes characters to write to 'dst'
 * 
 * @return : The number of bytes written to 'dst' (not including NULL terminator)
 */
int ucs_to_utf8(unsigned char *dst, const uchar *src, int max)
{
	int i = 0, ch;
	if (dst == NULL) {
		while ((ch = *src++)) {
			if (ch < 0x80) {
				i++;
			} else if (ch < 0x800) {
				i += 2;
			} else if (ch >= 0xD800 && ch <= 0xDFFF) {
				if (*src++ == 0)
					break;
				i += 4;
			} else {
				i += 3;
			}
		}
		return i;
	}
	int k, c2;
	while (i < max && (ch = *src++)) {
		if (ch < 0x80) {
			dst[i++] = ch;
		} else if (ch < 0x800) {
			dst[i++] = (0xC0 | (ch >> 6));
			dst[i++] = (0x80 | (ch & 63));
		} else if (ch >= 0xD800 && ch <= 0xDFFF) {
			c2 = *src++;
			if (!c2)
				break;
			k = (((ch - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
			dst[i++] = 0xF0 | (k >> 18);
			dst[i++] = 0x80 | ((k >> 12) & 63);
			dst[i++] = 0x80 | ((k >> 6) & 63);
			dst[i++] = 0x80 | (k & 63);
		} else {
			dst[i++] = 0xE0 | (ch >> 12);
			dst[i++] = 0x80 | ((ch >> 6) & 63);
			dst[i++] = 0x80 | (ch & 63);
		}
	}
	if (i < max)
		dst[i] = 0;
	return i;
}

/*
 * Convert utf-8 to uchar string.
 * 
 * @max : Maximum number of **uchar(wide)** characters to write to 'dst'.
 * 
 * @return : The number of **uchar(wide)** characters written to 'dst' (not including NULL terminator).
 */
int utf8_to_ucs(uchar *dst, const unsigned char *src, int max)
{
	int acc = 0, ch, c2, c3, c4;
	if (dst == NULL) {
		while ((ch = *src++)) {
			if (ch < 0x80) {
			} else if (ch < 0xE0) {
				if (!(0x80 & (*src++)))
					break;
			} else if (ch < 0xF0) {
				c2 = *src++;
				c3 = *src++;
				if (!(0x80 & c2 & c3))
					break;
			} else {
				c2 = *src++;
				c3 = *src++;
				c4 = *src++;
				if (!(0x80 & c2 & c3 & c4))
					break;
				acc += 2; // surrogate pair
				continue;
			}
			acc++;
		}
		return acc;
	}
	while ((acc < max) && (ch = *src++)) {
		if (ch < 0x80) {
		} else if (ch < 0xE0) {
			c2 = *src++;
			if (!(c2 & 0x80))
				break;
			ch = ((ch & 0x3F) << 6) | (c2 & 0x7F);
		} else if (ch < 0xF0) {
			c2 = *src++;
			c3 = *src++;
			if (!(c2 & c3 & 0x80))
				break;
			ch = ((ch & 0x1F) << 12) | ((c2 & 0x7F) << 6) | (c3 & 0x7F);
		} else {
			c2 = *src++;
			c3 = *src++;
			c4 = *src++;
			if (!(c2 & c3 & c4 & 0x80))
				break;
			ch = ((ch & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 & 0x7F) << 6) | (c4 & 0x7F);
			dst[acc++] = (ch >> 10) + 0xD7C0;
			dst[acc++] = (ch & 0x3FF) + 0xDC00;
			continue;
		}
		dst[acc++] = ch;
	}
	if (acc < max)
		dst[acc] = 0;
	return acc;
}
