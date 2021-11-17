CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Ofast
LIBS=
SRC=src/main.c src/recdir.c src/sha256.c

dedup: $(SRC)
	$(CC) $(CFLAGS) -o dedup $(SRC) $(LIBS)
