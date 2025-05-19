#

INC      := include
SRCDIR   := src
OBJDIR   := obj
EXE      := test.exe
LIB      := liblwm.a
CFLAGS   :=
LDFLAGS  :=
INCLUDES := -I$(INC)
OBJS     := buffer.o strbuf.o ucsbuf.o mempool.o crlf_counter.o \
            rstream.o pmap.o kuai.o ucs2.o
OBJS     := $(OBJS:%.o=$(OBJDIR)/%.o)

TESTOBJS := test.o kuai_test.o ucs2_test.o pmap_test.o buffer_test.o
TESTOBJS := $(TESTOBJS:%.o=$(OBJDIR)/%.o)


lib: $(LIB)

test: $(EXE) FORCE
	./$(EXE)

clean:
	rm -f $(EXE) $(LIB) $(OBJS) $(TESTOBJS) -d $(OBJDIR)

FORCE:;

.PHONY: lib test clean FORCE

# directory
$(OBJS): | $(OBJDIR)
$(TESTOBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(EXE): $(TESTOBJS) $(LIB)
	$(CC) $^ -o $@ $(LDFLAGS)

$(LIB): $(OBJS)
	ar rcs $@ $^

# lower-case vpath, NOTE: Don't uses vpath to match the generated file.
vpath %.h $(INC)
vpath %.c $(SRCDIR)

# test
$(OBJDIR)/test.o: test/test.c
$(OBJDIR)/pmap_test.o: test/pmap_test.c $(OBJDIR)/pmap.o
$(OBJDIR)/ucs2_test.o: test/ucs2_test.c $(OBJDIR)/ucs2.o
$(OBJDIR)/kuai_test.o: test/kuai_test.c $(OBJDIR)/kuai.o
$(OBJDIR)/buffer_test.o: test/buffer_test.c $(OBJDIR)/buffer.o $(OBJDIR)/strbuf.o $(OBJDIR)/crlf_counter.o

$(OBJDIR)/%.o: test/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

# .lib
$(OBJDIR)/%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ucs2.o: ucs2.c ucs2.h
$(OBJDIR)/pmap.o: pmap.c pmap.h
$(OBJDIR)/kuai.o: kuai.c kuai.h
$(OBJDIR)/buffer.o: buffer.c buffer.h
$(OBJDIR)/strbuf.o: strbuf.c strbuf.h
$(OBJDIR)/ucsbuf.o: ucsbuf.c ucsbuf.h
$(OBJDIR)/mempool.o: mempool.c mempool.h
$(OBJDIR)/crlf_counter.o: crlf_counter.c crlf_counter.h

#$(OBJDIR)/rstream.o: rstream.c rstream.h rlex.h
#$(OBJDIR)/rjson.o: rjson.c rjson.h
#$(OBJDIR)/rjson_parser_lex.o: rjson_parser_lex.c rjson.h
#$(OBJDIR)/rjson_parser_slr.o: rjson_parser_slr.c rjson.h
