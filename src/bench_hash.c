#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "./hash.h"

void usage(FILE *stream)
{
    fprintf(stream, "Usage: bench_hash <SHA256SUM>\n");
}

typedef void (*Hof_Func)(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash);

#define BUFFER_MAX_CAP (1024*1024*1024)

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))

typedef struct {
    const char *label;
    size_t value;
} Buffer_Cap_Attrib;

size_t buffer_caps[] = {
             1024,
         256*1024,
         512*1024,
        1024*1024,
    256*1024*1024,
    512*1024*1024,
};

typedef struct {
    const char *label;
    Hof_Func hof_func;
} Hof_Func_Attrib;

Hof_Func_Attrib hof_func_attribs[] = {
    {.label = "hash_of_file_libc",  .hof_func = hash_of_file_libc},
    {.label = "hash_of_file_linux", .hof_func = hash_of_file_linux},
    {.label = "hash_of_file_mmap",  .hof_func = hash_of_file_mmap},
};

#define HASH_LEN 64
#define SEP_LEN 2

typedef struct {
    const char *content_file_path;
    char *content;
    size_t content_size;
    size_t line_number;
} Hash_Sum_File;

Hash_Sum_File make_hash_sum_file(const char *content_file_path, char *content, size_t content_size)
{
    Hash_Sum_File result;
    result.content_file_path = content_file_path;
    result.content = content;
    result.content_size = content_size;
    result.line_number = 0;
    return result;
}

const char *next_hash_and_file(Hash_Sum_File *hsf, Hash *expected_hash)
{
    if (hsf->content_size > 0) {
        size_t line_size = strlen(hsf->content);

        if (line_size < HASH_LEN + SEP_LEN || !parse_hash(hsf->content, expected_hash)) {
            fprintf(stderr, "%s:%zu: ERROR: incorrect hash line\n",
                    hsf->content_file_path, hsf->line_number);
            exit(1);
        }

        const char *file_path = hsf->content + HASH_LEN + SEP_LEN;

        hsf->content += line_size + 1;
        hsf->content_size -= line_size + 1;

        return file_path;
    } else {
        return NULL;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "ERROR: no input file is provided\n");
        exit(1);
    }

    const char *content_file_path = argv[1];

    int fd = open(content_file_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n",
                content_file_path, strerror(errno));
        exit(1);
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(statbuf));
    if (fstat(fd, &statbuf) < 0) {
        fprintf(stderr, "ERROR: could not determine the size of the file %s: %s\n",
                content_file_path, strerror(errno));
        exit(1);
    }
    size_t content_size = statbuf.st_size;

    char *content = mmap(NULL, content_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (content == MAP_FAILED) {
        fprintf(stderr, "ERROR: could not memory map file %s: %s\n",
                content_file_path, strerror(errno));
        exit(1);
    }

    if (!(content_size > 0 && content[content_size - 1] == '\0')) {
        fprintf(stderr, "ERROR: the content of file %s is not correct. Expected the output of sha256sum(1) with -z flag.\n", content_file_path);
        exit(1);
    }

    uint8_t *buffer = malloc(BUFFER_MAX_CAP);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: could not allocate memory: %s\n",
                strerror(errno));
        exit(1);
    }

    printf("method,buffer-size,file-size,time-secs\n");
    for (size_t hof_func_index = 0;
            hof_func_index < ARRAY_LEN(hof_func_attribs);
            ++hof_func_index) {
        for (size_t buffer_cap_index = 0;
                buffer_cap_index < ARRAY_LEN(buffer_caps);
                ++buffer_cap_index) {
            Hof_Func hof = hof_func_attribs[hof_func_index].hof_func;
            const char *hof_label = hof_func_attribs[hof_func_index].label;
            size_t buffer_cap = buffer_caps[buffer_cap_index];

            Hash_Sum_File hsf = make_hash_sum_file(content_file_path, content, content_size);
            Hash expected_hash;
            const char *file_path = next_hash_and_file(&hsf, &expected_hash);
            while (file_path != NULL) {
                Hash actual_hash;
                // TODO: research how to gnuplot the results

                size_t file_size;
                {
                    struct stat statbuf;
                    if (stat(file_path, &statbuf) < 0) {
                        fprintf(stderr, "ERROR: could not determine the size of file %s: %s\n", 
                                file_path, strerror(errno));
                        exit(1);
                    }
                    file_size = statbuf.st_size;
                }

                struct timespec start, end;
                if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                    fprintf(stderr, "ERROR: could not get the current clock time: %s\n",
                            strerror(errno));
                    exit(1);
                }
                hof(file_path, buffer, buffer_cap, &actual_hash);
                if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
                    fprintf(stderr, "ERROR: could not get the current clock time: %s\n",
                            strerror(errno));
                    exit(1);
                }
                printf("%s,%zu,%zu,%ld.%09ld\n", 
                       hof_label, buffer_cap, file_size, 
                       end.tv_sec - start.tv_sec,
                       end.tv_nsec - start.tv_nsec);

                if (memcmp(&expected_hash, &actual_hash, sizeof(Hash)) != 0) {
                    fprintf(stderr, "ERROR: unexpected hash of file %s\n", file_path);
                    char hash_cstr[32*2 + 1];
                    hash_as_cstr(expected_hash, hash_cstr);
                    fprintf(stderr, "Expected: %s\n", hash_cstr);
                    hash_as_cstr(actual_hash, hash_cstr);
                    fprintf(stderr, "Actual:   %s\n", hash_cstr);
                    exit(1);
                }

                file_path = next_hash_and_file(&hsf, &expected_hash);
            }
        }
    }

    return 0;
}
