/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2025 LWM
 */

#ifndef LWM_UCS2_H
#define LWM_UCS2_H
#include <stddef.h>

#if (defined(_MSC_VER) && !defined(__llvm__)) || (__SIZEOF_WCHAR_T__ == 2) || (__WCHAR_MAX__ == 0xFFFF)
	#include <wchar.h>
	typedef wchar_t       uchar;
	#define USTR(str)      L##str
	#define usprintf      swprintf
	#define ucslen        wcslen
	#define ucscpy        wcscpy
	#define ucsncpy       wcsncpy
	#define ucscmp        wcscmp
	#define ucsncmp       wcsncmp
	#define ucscat        wcscat
	#define ucsncat       wcsncat
	#define ucschr        wcschr
	#define ucsrchr       wcsrchr
	#define ucsstr        wcsstr

	#define ucstod        wcstod
	#define ucstof        wcstof
	#define ucstol        wcstol

	#define UCHAR_NATIVE_FUN
#else
	#include <uchar.h>
	typedef char16_t     uchar;
	#define USTR(str)     u##str
	// int usprintf(uchar *const buff, const int count, const uchar *format, ...);
	int ucslen(const uchar *ucs);
	uchar *ucscpy(uchar *dst, const uchar *src);
	uchar *ucsncpy(uchar *dst, const uchar *src, unsigned int n);
	int ucscmp(const uchar *s1, const uchar *s2);
	int ucsncmp(const uchar *s1, const uchar *s2, unsigned int n);
	uchar *ucscat(uchar *dst, const uchar *src);
	uchar *ucsncat(uchar *dst, const uchar *src, unsigned int n);
	uchar *ucschr(const uchar* ucs, uchar ch);
	uchar *ucsrchr(const uchar* ucs, uchar ch);
	uchar *ucsstr(const uchar *ucs, const uchar *sub);

	double ucstod(const uchar *ucs, uchar **end);
	long ucstol(const uchar *ucs, uchar **end, int base);
	#define ucstof(u, e)  ((float)ucstod(u, e))
#endif

/*
 * Convert 'int' to uchar string
 */
int itoua(int value, uchar *out);
int uitoua(unsigned int value, uchar *out);
int uitoua16(unsigned int value, uchar *out);
int i64toua(long long value, uchar *out);
int u64toua(unsigned long long num, uchar *out);
int u64toua16(unsigned long long num, uchar *out);

/*
 * Convert 'float' to uchar string
 */
int ftoua(float value, uchar *out, int precision, int fixed);
int dtoua(double value, uchar *out, int precision, int fixed);

/*
 * uchar string <=> utf8 string
 */
int ucs_to_utf8(unsigned char *dst, const uchar *src, int max);
int utf8_to_ucs(uchar *dst, const unsigned char *src, int max);
#endif
