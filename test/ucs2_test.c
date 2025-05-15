#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <wchar.h>
#include <limits.h>
#include <math.h>
#include "ucs2.h"

void ucs2_test()
{
	uchar dst[128];
	uchar *ept = USTR("");
	uchar *src = USTR("01234567");

	assert(ucslen(ept) == 0);
	assert(ucslen(src) == 8);

	assert(ucscpy(dst, ept) == dst && ucscmp(dst, ept) == 0);
	assert(ucscpy(dst, src) == dst && ucscmp(dst, src) == 0);

	assert(ucsncpy(dst, src, 3) == dst && ucsncmp(dst, src, 3) == 0);
	assert(ucsncpy(dst, src, 6) == dst && ucsncmp(dst, src, 6) == 0);

	dst[0] = 0;
	assert(ucscat(dst, src) == dst && ucscmp(dst, src) == 0);
	
	dst[0] = 0;
	assert(ucsncat(dst, src, 3) == dst && ucscmp(dst, USTR("012")) == 0);

	assert(ucschr(src, '5') == src + 5 && ucschr(src, 'X') == NULL);
	assert(ucsrchr(src, '5') == src + 5 && ucsrchr(src, 'X') == NULL);

	assert(ucsstr(src, USTR("345")) == src + 3 && ucsstr(src, USTR("012")) == src && ucsstr(src, USTR("10")) == NULL);
	{ // itoua, uitoua16
		assert(itoua(101, dst) == 3 && ucscmp(dst, USTR("101")) == 0);
		assert(itoua(-10, dst) == 3 && ucscmp(dst, USTR("-10")) == 0);
		assert(itoua(0, dst) == 1 && ucscmp(dst, USTR("0")) == 0);
		assert(uitoua(-1, dst) == 10 && ucscmp(dst, USTR("4294967295")) == 0);
		assert(uitoua16(0xdeadbeef, dst) == 8 && ucscmp(dst, USTR("deadbeef")) == 0);
		assert(uitoua16(-1, dst) == 8 && ucscmp(dst, USTR("ffffffff")) == 0);

		assert(i64toua(9223372036854775807ULL, dst) == 19 && ucscmp(dst, USTR("9223372036854775807")) == 0);
		assert(i64toua(-9223372036854775808ULL, dst) == 20 && ucscmp(dst, USTR("-9223372036854775808")) == 0);
		assert(u64toua(-1, dst) == 20 && ucscmp(dst, USTR("18446744073709551615")) == 0);
		assert(u64toua16(-1, dst) == 16 && ucscmp(dst, USTR("ffffffffffffffff")) == 0);
	}
	{ // ucstol
		uchar *end;
		struct {
			uchar *s;
			int r;
		} list[] = {
			{USTR("-2147483648"), -2147483648}, // LONG_MIN
			{USTR("2147483647") ,  2147483647},  // LONG_MAX
			{USTR("0")          ,        0},
			{USTR("0x101")      ,    0x101},
			{USTR("1")          ,        1},
			{USTR("-1")         ,       -1},
		};
		int i = 0;
		while (i < sizeof(list) / sizeof(list[0])) {
			uchar *ucs = list[i].s;
			uchar *tmp = ucs + ucslen(ucs);
			assert(ucstol(ucs, &end, 0) == list[i].r);
			assert(end == tmp);
			i++;
		}
	}
	{ // ftoua
		struct {
			float v;
			uchar *s;
			int g; // precision : "%.*g"
			int f; // fixed : "%.*f"
		} list[] = {
			{NAN       , USTR("nan")     , -1, 0},
			{INFINITY  , USTR("inf")     , -1, 0},
			{-INFINITY , USTR("-inf")    , -1, 0},
			{3.1415926f, USTR("3.141593"),  7, 0},
			{1.f + 2.f , USTR("3.00")    , -1, 2},
			{0.0001234 , USTR("0.000123"),  3, 0},
			{-9.99995f , USTR("-10")     ,  5, 0},
			{9.99995f  , USTR("10")      ,  5, 0},
		};
		int i = 0;
		while (i < sizeof(list) / sizeof(list[0])) {
			assert(ftoua(list[i].v, dst, list[i].g, list[i].f) == ucslen(list[i].s));
			assert(ucscmp(dst, list[i].s) == 0);
			i++;
		}
	}
	{ // dtoua
		struct {
			double v;
			uchar *s;
			int g; // precision : "%.*g"
			int f; // fixed : %.*f
		} list[] = {
			{NAN       , USTR("nan")     , -1},
			{INFINITY  , USTR("inf")     , -1},
			{-INFINITY , USTR("-inf")    , -1},
			{3.141595  , USTR("3.1416")  ,  6},
			{3.0       , USTR("3.00")    , -1, 2},
			{0.0001234 , USTR("0.000123"),  3},
			{-9.99995  , USTR("-10")     ,  5},
			{9.99995   , USTR("10")      ,  5},
		};
		int i = 0;
		while (i < sizeof(list) / sizeof(list[0])) {
			assert(dtoua(list[i].v, dst, list[i].g, list[i].f) == ucslen(list[i].s));
			assert(ucscmp(dst, list[i].s) == 0);
			i++;
		}
	}
	{ // ucstoutf, utftoucs

	}
}
