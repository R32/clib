/*
* SPDX-License-Identifier: GPL-2.0
*/

#ifndef R32_TINYALLOC_H
#define R32_TINYALLOC_H
#include "comm.h"
C_FUNCTION_BEGIN

void tinyfree(struct slist_head* root, void* ptr);

void* tinyalloc(struct slist_head* root, int size);

void tinyreset(struct slist_head* root);

void tinydestroy(struct slist_head* root);

C_FUNCTION_END
#endif
