/*
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef R_UCS2_H
#define R_UCS2_H

#include "rclibs.h"
C_FUNCTION_BEGIN

/*
 * @param srclen/srcbytes : If this parameter is -1, the function processes the entire input string,
 * including the terminating null character. Therefore, the resulting string has a terminating null character,
 * and the length returned by the function includes this character
 *
 * If this parameter is set to a positive integer, the function processes exactly the specified
 * number of characters. If the provided size does not include a terminating null character,
 * the resulting string is not null-terminated, and the returned length does not include this character.
 */

int wcstoutf8(unsigned char *out, const unsigned short *src, int srclen);

int utf8towcs(unsigned short *out, const unsigned char *src, int srcbytes);

C_FUNCTION_END
#endif