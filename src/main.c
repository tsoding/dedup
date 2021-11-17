#define _DEFAULT_SOURCE
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./recdir.h"
#include "./sha256.h"
#define STB_DS_IMPLEMENTATION
#include "./stb_ds.h"

typedef struct {
    BYTE bytes[32];
} Hash;

void hash_as_cstr(Hash hash, char output[32*2 + 1])
{
    for (size_t i = 0; i < 32; ++i) {
        sprintf(&(output[i*2]), "%02x", (unsigned int) hash.bytes[i]);
    }
}

long int stat_file_size(const char *file_name)
{
    struct stat st;

    if (stat(file_name, &st) == 0)
        return st.st_size;
    else
        return -1;
}

void hash_of_file(const char *file_path, Hash *hash)
{
    SHA256_CTX ctx;
    memset(&ctx, 0, sizeof(ctx));
    sha256_init(&ctx);

    FILE *f = fopen(file_path, "rb");
    long int file_size = stat_file_size(file_path);

    if (f == NULL || file_size == -1) {
        fprintf(stderr, "Could not open file or find file size %s: %s\n",
                file_path, strerror(errno)); // fail on symbolic link
        exit(1);
    }

    if (file_size <= 2500000) { // crash on big files (stack size overflow)
        BYTE buffer[file_size];
            if (fread(buffer, sizeof(char), file_size, f) != (unsigned long) file_size) {
                fprintf(stderr, "Error reading file %s: %s\n", file_path, strerror(errno));
                exit(1);
            }
        sha256_update(&ctx, buffer, file_size);
    } else {
        BYTE *buffer = malloc(sizeof(char) * file_size);
            if (fread(buffer, sizeof(char), file_size, f) != (unsigned long) file_size) {
                fprintf(stderr, "Error reading file %s: %s\n", file_path, strerror(errno));
                exit(1);
            }
        sha256_update(&ctx, buffer, file_size);
        free(buffer);
    }

    if (ferror(f)) {
        fprintf(stderr, "Could not read from file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);

    sha256_final(&ctx, hash->bytes);
}

typedef struct {
    Hash key;
    char **paths;
} Record;

Record *db = NULL;

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    // TODO: some sort of parallelization

    RECDIR *recdir = recdir_open(".");

    errno = 0;
    struct dirent *ent = recdir_read(recdir);
    while (ent) {
        Hash hash;
        char *path = join_path(recdir_top(recdir)->path, ent->d_name);
        hash_of_file(path, &hash);

        ptrdiff_t index = hmgeti(db, hash);
        if (index < 0) {
            Record record;
            record.key = hash;
            record.paths = NULL;
            arrput(record.paths, path);
            hmputs(db, record);
        } else {
            arrput(db[index].paths, path);
        }
        ent = recdir_read(recdir);
    }

    if (errno != 0) {
        fprintf(stderr,
                "ERROR: could not read the directory: %s\n",
                recdir_top(recdir)->path);
        exit(1);
    }

    recdir_close(recdir);

    for (ptrdiff_t i = 0; i < hmlen(db); ++i) {
        if (arrlen(db[i].paths) > 1) {
            char output[32*2 + 1];
            hash_as_cstr(db[i].key, output);
            printf("%s\n", output);
            for (ptrdiff_t j = 0; j < arrlen(db[i].paths); ++j) {
                printf("    %s\n", db[i].paths[j]);
            }
        }
        for (ptrdiff_t j = 0; j < arrlen(db[i].paths); ++j) {
            free(db[i].paths[j]);
        }
        arrfree(db[i].paths);
    }
    hmfree(db);
    return 0;
}
