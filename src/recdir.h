#ifndef RECDIR_H_
#define RECDIR_H_

#include <stdlib.h>

#include <dirent.h>

#define RECDIR_STACK_CAP 1024

#define PATH_SEP "/"
#define PATH_SEP_LEN (sizeof(PATH_SEP) - 1)

typedef struct {
    DIR *dir;
    char *path;
} RECDIR_Frame;

typedef struct {
    RECDIR_Frame stack[RECDIR_STACK_CAP];
    size_t stack_size;
} RECDIR;

char *join_path(const char *base, const char *file);

int recdir_push(RECDIR *recdir, char *path);
void recdir_pop(RECDIR *recdir);
char *recdir_path(RECDIR *recdir);

RECDIR *recdir_open(const char *dir_path);
struct dirent *recdir_read(RECDIR *recdirp);
void recdir_close(RECDIR *recdirp);

#endif // RECDIR_H_
