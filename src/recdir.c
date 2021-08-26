#define _DEFAULT_SOURCE
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "./recdir.h"

char *join_path(const char *base, const char *file)
{
    size_t base_len = strlen(base);
    size_t file_len = strlen(file);

    // TODO: avoid using malloc in here
    char *begin = malloc(base_len + file_len + PATH_SEP_LEN + 1);
    assert(begin != NULL);

    char *end = begin;
    memcpy(end, base, base_len);
    end += base_len;
    memcpy(end, PATH_SEP, PATH_SEP_LEN);
    end += PATH_SEP_LEN;
    memcpy(end, file, file_len);
    end += file_len;
    *end = '\0';

    return begin;
}

char *recdir_path(RECDIR *recdir)
{
    assert(recdir->stack_size > 0);
    return recdir->stack[recdir->stack_size - 1].path;
}

int recdir_push(RECDIR *recdir, char *path)
{
    assert(recdir->stack_size < RECDIR_STACK_CAP);
    RECDIR_Frame *top = &recdir->stack[recdir->stack_size];
    top->path = path;
    top->dir = opendir(top->path);
    if (top->dir == NULL) {
        free(top->path);
        return -1;
    }
    recdir->stack_size++;
    return 0;
}

void recdir_pop(RECDIR *recdir)
{
    assert(recdir->stack_size > 0);
    RECDIR_Frame *top = &recdir->stack[--recdir->stack_size];
    int ret = closedir(top->dir);
    assert(ret == 0);
    free(top->path);
}

RECDIR *recdir_open(const char *dir_path)
{
    RECDIR *recdir = malloc(sizeof(RECDIR));
    assert(recdir != NULL);
    memset(recdir, 0, sizeof(RECDIR));

    if (recdir_push(recdir, strdup(dir_path)) < 0) {
        free(recdir);
        return NULL;
    }

    return recdir;
}

struct dirent *recdir_read(RECDIR *recdirp)
{
    while (recdirp->stack_size > 0) {
        RECDIR_Frame *top = &recdirp->stack[recdirp->stack_size - 1];
        struct dirent *ent = readdir(top->dir);
        if (ent) {
            if (ent->d_type == DT_DIR) {
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                    continue;
                } else {
                    recdir_push(recdirp, join_path(top->path, ent->d_name));
                    continue;
                }
            } else {
                return ent;
            }
        } else {
            if (errno) {
                return NULL;
            } else {
                recdir_pop(recdirp);
                continue;
            }
        }
    }

    return NULL;
}

void recdir_close(RECDIR *recdirp)
{
    while (recdirp->stack_size > 0) {
        recdir_pop(recdirp);
    }
    free(recdirp);
}
