# TODO: not compilable by GCC 8.3.0
CC=clang
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb -O3
LIBS=
SRC=src/main.c src/recdir.c src/sha256.c src/hash.c

dedup: $(SRC)
	$(CC) $(CFLAGS) -o dedup $(SRC) $(LIBS)

bench_hash: src/bench_hash.c src/sha256.c src/hash.c
	$(CC) $(CFLAGS) -o bench_hash src/bench_hash.c src/sha256.c src/hash.c

gargen: src/gargen.c
	$(CC) $(CFLAGS) -o gargen src/gargen.c

.PHONY: bench
bench: bench_hash SHA256SUM
	./bench_hash SHA256SUM

BINS=input1K.bin input1M.bin

SHA256SUM: $(BINS)
	sha256sum -z $(BINS) > SHA256SUM

input1K.bin: gargen
	./gargen -file-size 1K input1K.bin

input1M.bin: gargen
	./gargen -file-size 1M input1M.bin
