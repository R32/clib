# cygwin
# VERSION := @$(shell git rev-parse --short HEAD)
INC      := include
SRC      := src
OBJ      := obj
EXE      := test.exe
LIB      := libr32c.a
CFLAGS   := -fshort-wchar
INCLUDES := -I$(INC)
OBJS     := slist.o rbtree.o ucs2.o tinyalloc.o strbuf.o wcsbuf.o rarray.o \
            rstream.o crlf_counter.o

ifdef mingw
    CC   := i686-w64-mingw32-gcc
else
    CC   := gcc
endif


lib: $(OBJ) $(LIB)

test: $(OBJ) $(EXE) FORCE
	./$(EXE)

clean:
	rm -rf $(EXE) $(LIB) $(OBJ)

FORCE:;

.PHONY: all clean lib FORCE

$(OBJ):
	@mkdir -p $@

$(EXE): $(OBJ)/main.o $(LIB)
	$(CC) $(INCLUDES) $(CFLAGS) $< -L$(dir $(LIB)) -lr32c -o $@

$(LIB): $(OBJS:%.o=$(OBJ)/%.o)
	ar rcs $@ $^

# lower-case vpath, NOTE: Don't uses vpath to match the generated file.
vpath %.h $(INC)
vpath %.c $(SRC)

# .exe
$(OBJ)/main.o: main.c rclibs.h

# .lib
$(OBJ)/%.o: %.c rclibs.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OBJ)/ucs2.o: ucs2.c ucs2.h

$(OBJ)/slist.o: slist.c slist.h

$(OBJ)/rbtree.o: rbtree.c rbtree.h rbtree_augmented.h

$(OBJ)/tinyalloc.o: tinyalloc.c tinyalloc.h

$(OBJ)/strbuf.o: strbuf.c strbuf.h
$(OBJ)/wcsbuf.o: wcsbuf.c wcsbuf.h
$(OBJ)/rarray.o: rarray.c rarray.h
$(OBJ)/rstream.o: rstream.c rstream.h rlex.h
$(OBJ)/crlf_counter.o: crlf_counter.c crlf_counter.h