#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./recdir.h"
#include "./sha256.h"
#define STB_DS_IMPLEMENTATION
#include "./stb_ds.h"
#include "./hash.h"

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

    BYTE buffer[1024];

    errno = 0;
    struct dirent *ent = recdir_read(recdir);
    while (ent) {
        Hash hash;
        char *path = join_path(recdir_top(recdir)->path, ent->d_name);
        hash_of_file_libc(path, buffer, sizeof(buffer), &hash);

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

    // TODO: memory allocated by the iterations process is not cleaned up properly
    // - join_path
    // - dynamic array of paths
    // - hash table

    for (ptrdiff_t i = 0; i < hmlen(db); ++i) {
        if (arrlen(db[i].paths) > 1) {
            char output[32*2 + 1];
            hash_as_cstr(db[i].key, output);
            printf("%s\n", output);

            for (ptrdiff_t j = 0; j < arrlen(db[i].paths); ++j) {
                printf("    %s\n", db[i].paths[j]);
            }
        }
    }

    return 0;
}
