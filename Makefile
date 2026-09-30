CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Isrc
LDFLAGS := -lSDL2

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

emu: $(OBJ)
	$(CC) $(OBJ) -o emu $(LDFLAGS)

tests/test_alu: tests/test_alu.c src/alu.c src/flags.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean test

test: tests/test_alu
	./tests/test_alu

clean:
	rm -f src/*.o emu tests/test_alu
