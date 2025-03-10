my c language trashbox
--------

- [`tinyalloc`](src/tinyalloc.c) : Releases all requested memory at once instead of releasing each object separately.

  * tinyalloc : The requested memory block can be freed individually.
  * bumpalloc : The requested memory block cannot be freed individually, you can only call `bumpreset/bumpdestroy` to free all blocks at once
  * fixedalloc :

- [`strbuf`](src/strbuf.c): Auto-growing string buffer

  * [`wcsbuf`](src/wcsbuf.c) : The wchar_t version of strbuf

- [`pmap`](src/pmap.c) : PMap in C language, This code is ported from OCaml ExtLib PMap [sample](test/pmap_test.c)

- ~~[`rarray`](src/rarray.c) : Auto-growing arrays(by `realloc`)~~ It's horrible

  <details><summary>hiden</summary>
  ```c
  // rarray_fast_set, rarray_fast_get
  struct point {
      int x, y, z;
  };
  struct rarray array = { .size = sizeof(struct point), .base = NULL };
  int len = 16;
  // you could also call `rarray_grow()` to increase "capacity" only
  rarray_setlen(&array, len);
  struct point *ptr = rarray_fast_get(&array, struct point, 0);
  for (int i = 0; i < len; i++) {
      *ptr++ = (struct point){ i, i, i };
  }
  for (int i = 0; i < len; i++) {
      struct point *ptr = rarray_fast_get(&array, struct point, i);
      assert(ptr->x == i && ptr->y == i && ptr->z == i);
  }
  rarray_discard(&array);
  ```
  </details>

- ~~`slist.h`: Singly Linked List.~~ Deprecated

- `ucs2`: wcs_to_utf8, utf8_to_wcs

- [`rjson`](src/rjson.c) :

- `circ_buf.h`: Copied from [linux/circ_buf.h](https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h)

- `list.h`: Doubly Linked List. Copied from [linux/tools/list.h](https://github.com/torvalds/linux/blob/master/tools/include/linux/list.h)

- `rbtree`: Copied from [linux/tools/rbtree.h](https://github.com/torvalds/linux/blob/master/tools/include/linux/rbtree.h)

## external links

- [lexer and Simple LR tool](https://github.com/r32/lex)
