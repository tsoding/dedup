#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./recdir.h"

// TODO: compute the hashes of the files
// TODO: build the hash table
int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    RECDIR *recdir = recdir_open(".");

    errno = 0;
    struct dirent *ent = recdir_read(recdir);
    while (ent) {
        printf("recdir file: %s/%s\n", recdir_path(recdir), ent->d_name);
        ent = recdir_read(recdir);
    }

    if (errno != 0) {
        fprintf(stderr,
                "ERROR: could not read the directory: %s\n",
                recdir_path(recdir));
        exit(1);
    }

    recdir_close(recdir);

    return 0;
}
