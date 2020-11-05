# cygwin
# VERSION := @$(shell git rev-parse --short HEAD)
INC      := include
SRC      := src
OBJ      := obj
EXE      := test.exe
LIB      := libr32c.a
CFLAGS   := -fshort-wchar
INCLUDES := -I$(INC)

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

$(LIB): $(addprefix $(OBJ)/,slist.o rbtree.o ucs2.o)
	ar rcs $@ $^

# lower-case vpath, NOTE: Don't uses vpath to match the generated file.
vpath %.h $(INC)
vpath %.c $(SRC)

# .exe
$(OBJ)/main.o: main.c comm.h

# .lib
$(OBJ)/%.o: %.c comm.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OBJ)/ucs2.o: ucs2.c ucs2.h

$(OBJ)/slist.o: slist.c slist.h

$(OBJ)/rbtree.o: rbtree.c rbtree.h rbtree_augmented.h
