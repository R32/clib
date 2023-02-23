#
ROOT     := ../../../
LIB      := $(ROOT)libr32c.a
INCLUDES := -I$(ROOT)include
CFLAGS   := -fshort-wchar
OUTDIR   := ../..
# make dump=1
DUMP     := $(if $(value dump), -fdump-tree-optimized -O3,)

all: main.exe
	@./$<

main.exe: main.c rjson_parser_slr.o rjson_parser_lex.o
	@gcc $(INCLUDES) $(CFLAGS) $(DUMP) $^ -L$(ROOT) -lr32c -o $@

rjson_parser_slr.o: $(OUTDIR)/rjson_parser_slr.c
	@gcc $(INCLUDES) $(CFLAGS) $(DUMP) -c $< -L$(ROOT) -lr32c -o $@
rjson_parser_lex.o: $(OUTDIR)/rjson_parser_lex.c
	@gcc $(INCLUDES) $(CFLAGS) $(DUMP) -c $< -L$(ROOT) -lr32c -o $@

$(OUTDIR)/rjson_parser_slr.c $(OUTDIR)/rjson_parser_lex.c: rjson_parser.slr rjson_parser.lex
	haxelib run lex --out $(OUTDIR) --slr $^

clean:
	rm -f *.o

.PHONY: test clean FORCE
