# cygwin
# VERSION := @$(shell git rev-parse --short HEAD)
INC      := include
SRC      := src
OBJ      := obj
EXE      := test.exe
LIB      := $(OBJ)/libr32c.a
CFLAGS   := -fshort-wchar
INCLUDES := -I$(INC)

ifdef mingw
    CC   := i686-w64-mingw32-gcc
else
    CC   := gcc
endif

COMM_H   := $(INC)/comm.h

lib: $(OBJ) $(LIB)

test: $(OBJ) $(EXE) FORCE
	./$(EXE)

clean:
	rm -rf *.exe $(OBJ)

FORCE:;

.PHONY: all clean lib FORCE

$(EXE): $(OBJ)/main.o $(LIB)
	$(CC) $(INCLUDES) $(CFLAGS) $< -L$(OBJ) -lr32c -o $@

$(LIB): $(addprefix $(OBJ)/, slist.o)
	ar rcs $@ $^

$(OBJ):
	@mkdir -p $@

# obj/*.o
$(OBJ)/main.o: main.c $(COMM_H)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OBJ)/slist.o: $(SRC)/slist.c $(INC)/slist.h $(COMM_H)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

