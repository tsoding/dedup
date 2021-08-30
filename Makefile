# TODO: not compilable by GCC 8.3.0
CC=clang
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb -O3
LIBS=
SRC=src/main.c src/recdir.c src/sha256.c src/hash.c

dedup: $(SRC)
	$(CC) $(CFLAGS) -o dedup $(SRC) $(LIBS)

bench_hash: src/bench_hash.c src/sha256.c src/hash.c
	$(CC) $(CFLAGS) -o bench_hash src/bench_hash.c src/sha256.c src/hash.c -lm

gargen: src/gargen.c
	$(CC) $(CFLAGS) -o gargen src/gargen.c

.PHONY: bench
bench: bench_hash SHA256SUM
	./bench_hash SHA256SUM | tee "bench_hash.report"

BINS=input1K.bin input256K.bin input512K.bin input1M.bin 

SHA256SUM: $(BINS)
	sha256sum -z $(BINS) > SHA256SUM

input1K.bin: gargen
	./gargen -file-size 1K input1K.bin

input256K.bin: gargen
	./gargen -file-size 256K input256K.bin

input512K.bin: gargen
	./gargen -file-size 512K input512K.bin

input1M.bin: gargen
	./gargen -file-size 1M input1M.bin

input256M.bin: gargen
	./gargen -file-size 256M input256M.bin

input512M.bin: gargen
	./gargen -file-size 512M input512M.bin
