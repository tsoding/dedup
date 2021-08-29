#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "./hash.h"
#define PROF
#include "./prof.c"

void usage(FILE *stream)
{
    fprintf(stream, "Usage: bench_hash <SHA256SUM>\n");
}

typedef void (*Hof_Func)(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash);

void bench_file(const char *file_path, Hash expected_hash, BYTE *buffer, size_t buffer_cap, Hof_Func hof)
{
    Hash actual_hash;
    begin_clock(file_path);
    hof(file_path, buffer, buffer_cap, &actual_hash);
    end_clock();

    if (memcmp(&expected_hash, &actual_hash, sizeof(Hash)) != 0) {
        fprintf(stderr, "Checksum failed on file %s\n", file_path);
        char output[32*2 + 1];
        hash_as_cstr(actual_hash, output);
        fprintf(stderr, "Actual:   %s\n", output);
        hash_as_cstr(expected_hash, output);
        fprintf(stderr, "Expected: %s\n", output);
        exit(1);
    }
}

#define BUFFER_MAX_CAP (1000*1000*1000)

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))

typedef struct {
    const char *label;
    size_t value;
} Buffer_Cap_Attrib;

Buffer_Cap_Attrib buffer_cap_attribs[] = {
    {.label = "CAPACITY SMOL", .value = 1024},
    {.label = "CAPACITY HALF", .value = BUFFER_MAX_CAP / 2},
    {.label = "CAPACITY FULL", .value = BUFFER_MAX_CAP},
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

bool parse_hexdigit(char c, uint8_t *digit)
{
    if ('0' <= c && c <= '9') *digit = c - '0';
    else if ('a' <= c && c <= 'f') *digit = c - 'a' + 10;
    else if ('A' <= c && c <= 'F') *digit = c - 'a' + 10;
    else return false;
    return true;
}

bool parse_hash(const char input[HASH_LEN], Hash *output)
{
    for (size_t i = 0; i < 32; ++i) {
        uint8_t a, b;
        if (!parse_hexdigit(input[2*i + 0], &a)) return false;
        if (!parse_hexdigit(input[2*i + 1], &b)) return false;
        output->bytes[i] = a * 0x10 + b;
    }
    return true;
}

void process_content(const char *content_file_path, char *content, size_t content_size, 
                     Hof_Func hof, BYTE *buffer, size_t buffer_cap)
{
    for (size_t line_number = 0; content_size > 0; ++line_number) {
        size_t line_size = strlen(content);

        Hash expected_hash;
        if (line_size < HASH_LEN + SEP_LEN || !parse_hash(content, &expected_hash)) {
            fprintf(stderr, "%s:%zu: ERROR: incorrect hash line\n",
                    content_file_path, line_number);
            exit(1);
        }

        const char *file_path = content + HASH_LEN + SEP_LEN;
        
        Hash actual_hash;
        begin_clock(file_path);
        hof(file_path, buffer, buffer_cap, &actual_hash);
        end_clock();

        if (memcmp(&expected_hash, &actual_hash, sizeof(Hash)) != 0) {
            fprintf(stderr, "ERROR: unexpected hash of file %s\n", file_path);
            char hash_cstr[32*2 + 1];
            hash_as_cstr(expected_hash, hash_cstr);
            fprintf(stderr, "Expected: %s\n", hash_cstr);
            hash_as_cstr(actual_hash, hash_cstr);
            fprintf(stderr, "Actual:   %s\n", hash_cstr);
            exit(1);
        }

        content += line_size + 1;
        content_size -= line_size + 1;
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

    begin_clock("TOTAL");
    {
        for (size_t hof_func_index = 0;
                hof_func_index < ARRAY_LEN(hof_func_attribs);
                ++hof_func_index) {
            begin_clock(hof_func_attribs[hof_func_index].label);
            for (size_t buffer_cap_index = 0;
                    buffer_cap_index < ARRAY_LEN(buffer_cap_attribs);
                    ++buffer_cap_index) {
                Hof_Func hof = hof_func_attribs[hof_func_index].hof_func;
                size_t buffer_cap = buffer_cap_attribs[buffer_cap_index].value;

                begin_clock(buffer_cap_attribs[buffer_cap_index].label);
                process_content(content_file_path, content, content_size, hof, buffer, buffer_cap);
                end_clock();
            }
            end_clock();
        }
    }
    end_clock();

    dump_summary(stderr);

    return 0;
}
