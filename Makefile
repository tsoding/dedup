# TODO: not compilable by GCC 8.3.0
CC=clang
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb -O3
LIBS=
SRC=src/main.c src/recdir.c src/sha256.c src/hash.c

.PHONY: all
all: dedup bench_hash gargen

dedup: $(SRC)
	$(CC) $(CFLAGS) -o dedup $(SRC) $(LIBS)

bench_hash: src/bench_hash.c src/sha256.c src/hash.c
	$(CC) $(CFLAGS) -o bench_hash src/bench_hash.c src/sha256.c src/hash.c

gargen: src/gargen.c
	$(CC) $(CFLAGS) -o gargen src/gargen.c
