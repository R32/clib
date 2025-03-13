/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 * Copyright (C) 2025 Liuwm
 */
/*
 * PMap - Polymorphic maps
 * Copyright (C) 1996-2003 Xavier Leroy, Nicolas Cannasse, Markus Mottl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version,
 * with the special exception on linking described in file LICENSE.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * PMap in C language, This code is ported from OCaml ExtLib PMap
 */
#ifndef R32_PMAP_H
#define R32_PMAP_H

struct pmnode {
	struct pmnode *left;
	struct pmnode *right;
	int height;
};

int pmap_count(struct pmnode *root);
void pmap_balance(struct pmnode **slot, int *breakout);
struct pmnode *pmap_merge(struct pmnode **slot);

static int inline pmap_height(struct pmnode *node)
{
	return node ? node->height : 0;
}

#ifndef NULL
#   define NULL 0
#endif

#ifndef container_of
#   if defined(_MSC_VER) || !defined(__llvm__) // unsafe in msvc
#       define container_of(ptr, type, member)\
        ((type *)((char *)ptr - offsetof(type, member)))
#   else
#   define container_of(ptr, type, member) ({\
        const __typeof__(((type *)0)->member) * __mptr = (ptr);\
        (type *)((char *)ptr - offsetof(type, member)); })
#   endif
#endif


#ifndef VLADecl
#   ifdef _MSC_VER
#       define VLADecl(type, name, len) type *name = _alloca(sizeof(type) * (len))
#else
#       define VLADecl(type, name, len) type name[len]
#   endif
#endif

// three-level pointer : `struct pmnode **name[len + 1]`
#define pmap_stacks_decl(name, len) VLADecl(struct pmnode **, name, len + 1)

#endif
