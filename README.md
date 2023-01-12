clib
--------

- `tinyalloc`: Releases all requested memory at once instead of releasing each object separately.

  * tinyalloc : The requested memory block can be freed individually.
  * bumpalloc : The requested memory block cannot be freed individually, you can only call `bumpreset/bumpdestroy` to free all blocks at once
  * fixedalloc :

- `strbuf`: string buffer

- `slist.h`: Singly Linked List.

- `ucs2`: wcs_to_utf8, utf8_to_wcs

- `circ_buf.h`: Copied from [linux/circ_buf.h](https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h)

- `list.h`: Doubly Linked List. Copied from [linux/tools/list.h](https://github.com/torvalds/linux/blob/master/tools/include/linux/list.h)

- `rbtree`: Copied from [linux/tools/rbtree.h](https://github.com/torvalds/linux/blob/master/tools/include/linux/rbtree.h)

## external links

- [simple lexer tool](https://github.com/r32/lex) is used for simple regexp or token
