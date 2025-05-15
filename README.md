my c language trashbox
--------

- `ucs2.c`: `short wchar_t` routines(equivalent to `wchar_t` in MSVC, `char16_t` for others)

- [`pmap`](src/pmap.c) : PMap(avl tree) in C language, This code is ported from OCaml ExtLib PMap [sample](test/pmap_test.c)

- [`buffer.c`] : auto-growing chunk allocator

  - [`strbuf`](src/strbuf.c): auto-growing ansi string buffer

  - [`ucsbuf`](src/ucsbuf.c) : The uchar version of strbuf

  - [`mempool`](src/mempool.c) : fixed-size memory block allocator

  - [`crlf_counter`(src/crlf_counter.c)] for lexer/parser to track the '\n' position

- [`kuai.c`](src/kuai.c) : A lightweight first-class heaps allocator.

- [`rjson`](src/rjson.c) :

- `circ_buf.h`: Copied from [linux/circ_buf.h](https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h)

- `list.h`: Doubly Linked List. Copied from [linux/tools/list.h](https://github.com/torvalds/linux/blob/master/tools/include/linux/list.h)

- ~~[`tinyalloc`](src/tinyalloc.c) : Releases all requested memory at once instead of releasing each object separately.~~ instead of by `kuai.c`

- ~~`slist.h`: Singly Linked List.~~ Deprecated

## external links

- [lexer and Simple LR tool](https://github.com/r32/lex)
