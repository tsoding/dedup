#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./recdir.h"
#include "./sha256.h"

char hex_digit(unsigned int digit)
{
    digit = digit % 0x10;
    if (digit <= 9) return digit + '0';
    if (10 <= digit && digit <= 15) return digit - 10 + 'a';
    assert(0 && "unreachable");
}

void hash_as_cstr(BYTE hash[32], char output[32*2 + 1])
{
    for (size_t i = 0; i < 32; ++i) {
        output[i*2 + 0] = hex_digit(hash[i] / 0x10);
        output[i*2 + 1] = hex_digit(hash[i]);
    }
    output[32*2] = '\0';
}

void hash_of_file(const char *file_path, BYTE hash[32])
{
    SHA256_CTX ctx;
    memset(&ctx, 0, sizeof(ctx));
    sha256_init(&ctx);

    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Could not open file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    BYTE buffer[1024];
    size_t buffer_size = fread(buffer, 1, sizeof(buffer), f);
    while (buffer_size > 0) {
        sha256_update(&ctx, buffer, buffer_size);
        buffer_size = fread(buffer, sizeof(buffer), 1, f);
    }

    if (ferror(f)) {
        fprintf(stderr, "Could not read from file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);

    sha256_final(&ctx, hash);
}

// TODO: build the hash table
int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    RECDIR *recdir = recdir_open(".");

    errno = 0;
    struct dirent *ent = recdir_read(recdir);
    while (ent) {
        BYTE hash[32];
        char hash_cstr[32*2 + 1];
        char *path = join_path(recdir_top(recdir)->path, ent->d_name);
        hash_of_file(path, hash);
        hash_as_cstr(hash, hash_cstr);
        printf("%s %s\n", hash_cstr, path);
        free(path);

        ent = recdir_read(recdir);
    }

    if (errno != 0) {
        fprintf(stderr,
                "ERROR: could not read the directory: %s\n",
                recdir_top(recdir)->path);
        exit(1);
    }

    recdir_close(recdir);

    return 0;
}
