#ifndef R32_UCS2_H
#define R32_UCS2_H

#include "comm.h"
C_FUNCTION_BEGIN
/*
NOTE:
 1. You should call `setlocale(LC_CTYPE, "")` before using.
 2. You might have to add '\0' at end of `out`.
*/

int wcstoutf8(unsigned char* out, const wchar_t* src);

int utf8towcs(wchar_t* out, const unsigned char* src);

C_FUNCTION_END
#endif