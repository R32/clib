/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_UCS2_H
#define R_UCS2_H

#include "rclibs.h"
C_FUNCTION_BEGIN

/*
**NOTE: You have to add '\0' at end of `out`.
*/

int wcstoutf8(unsigned char *out, const wchar_t *src, int len);

int utf8towcs(wchar_t *out, const unsigned char *src, int len);

C_FUNCTION_END
#endif