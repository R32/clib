# cygwin
# VERSION := @$(shell git rev-parse --short HEAD)
INC      := include
SRC      := src
OBJ      := obj
EXE      := test.exe
LIB      := liblwm.a
CFLAGS   :=
INCLUDES := -I$(INC)
OBJS     := buffer.o strbuf.o ucsbuf.o mempool.o crlf_counter.o \
            rstream.o pmap.o kuai.o ucs2.o


TEST_OBJS:= kuai_test.o ucs2_test.o pmap_test.o buffer_test.o

ifdef mingw
    CC   := i686-w64-mingw32-gcc
else
    CC   := gcc
endif


lib: $(OBJ) $(LIB)

test: $(EXE) FORCE
	./$(EXE)

clean:
	rm -rf $(EXE) $(LIB) $(OBJ)

FORCE:;

.PHONY: all clean lib test FORCE

$(OBJ):
	@mkdir -p $@

$(EXE): $(OBJ)/test.o $(TEST_OBJS:%.o=$(OBJ)/%.o) $(LIB)
	$(CC) $(INCLUDES) $(CFLAGS) $^ -o $@

$(LIB): $(OBJS:%.o=$(OBJ)/%.o)
	ar rcs $@ $^

# lower-case vpath, NOTE: Don't uses vpath to match the generated file.
vpath %.h $(INC)
vpath %.c $(SRC)

# .exe
$(OBJ)/test.o: test/test.c

# test
$(OBJ)/pmap_test.o: test/pmap_test.c $(OBJ)/pmap.o
$(OBJ)/ucs2_test.o: test/ucs2_test.c $(OBJ)/ucs2.o
$(OBJ)/kuai_test.o: test/kuai_test.c $(OBJ)/kuai.o
$(OBJ)/buffer_test.o: test/buffer_test.c $(OBJ)/buffer.o $(OBJ)/strbuf.o $(OBJ)/crlf_counter.o

$(OBJ)/%.o: test/%.c | $(OBJ)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

# .lib
$(OBJ)/%.o: %.c | $(OBJ)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OBJ)/ucs2.o: ucs2.c ucs2.h
$(OBJ)/pmap.o: pmap.c pmap.h
$(OBJ)/kuai.o: kuai.c kuai.h
$(OBJ)/buffer.o: buffer.c buffer.h
$(OBJ)/strbuf.o: strbuf.c strbuf.h
$(OBJ)/ucsbuf.o: ucsbuf.c ucsbuf.h
$(OBJ)/mempool.o: mempool.c mempool.h
$(OBJ)/crlf_counter.o: crlf_counter.c crlf_counter.h

#$(OBJ)/rstream.o: rstream.c rstream.h rlex.h
#$(OBJ)/rjson.o: rjson.c rjson.h
#$(OBJ)/rjson_parser_lex.o: rjson_parser_lex.c rjson.h
#$(OBJ)/rjson_parser_slr.o: rjson_parser_slr.c rjson.h