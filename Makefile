CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb
LIBS=
SRC=src/main.c src/recdir.c src/sha256.c

dedup: $(SRC)
	$(CC) $(CFLAGS) -o dedup $(SRC) $(LIBS)
