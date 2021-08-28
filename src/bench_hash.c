#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "./hash.h"

#define PROF
#include "./prof.c"

// ./bench_hash input1K.bin input1M.bin input1G.bin
void usage(FILE *stream)
{
    fprintf(stream, "Usage: bench_hash <FILES...>\n");
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
    const char *file_path;
    Hash expected_hash;
} Input_File_Attrib;

// TODO: make bench_hash read input from SHA256SUM files
Input_File_Attrib input_file_attribs[] = {
    {"input1K.bin",  {{0x8f, 0x2f, 0x52, 0x61, 0xdc, 0xa5, 0xbd, 0x9e, 0x3d, 0xec, 0x81, 0x76, 0x7f, 0xd4, 0x46, 0xdc, 0xd7, 0xeb, 0xa1, 0x31, 0x7f, 0x3b, 0x49, 0xbd, 0x0b, 0xc9, 0x8d, 0x40, 0x96, 0x5e, 0x32, 0xf4}}},
    {"input1M.bin",  {{0x2e, 0x1a, 0xb8, 0x54, 0x90, 0xcb, 0xec, 0x88, 0xfd, 0x62, 0x96, 0xcf, 0x2f, 0x9a, 0x1f, 0xb9, 0x50, 0xd9, 0x82, 0xa3, 0x00, 0xf1, 0x00, 0xba, 0x00, 0xeb, 0x5e, 0xa4, 0x41, 0x88, 0x44, 0xc7}}},
    {"input1G.bin",  {{0x73, 0x6d, 0x4d, 0x43, 0x65, 0xc5, 0x2e, 0x9a, 0xac, 0x39, 0xcc, 0xca, 0x7f, 0x49, 0xba, 0x76, 0xe1, 0x05, 0x4c, 0x40, 0x12, 0xef, 0xaa, 0xce, 0xbb, 0x65, 0xc7, 0x90, 0x4b, 0xc8, 0x02, 0xef}}},
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

int main(void)
{
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
                begin_clock(buffer_cap_attribs[buffer_cap_index].label);
                for (size_t input_file_index = 0;
                        input_file_index < ARRAY_LEN(input_file_attribs);
                        ++input_file_index) {
                    const char *file_path = input_file_attribs[input_file_index].file_path;
                    Hash expected_hash = input_file_attribs[input_file_index].expected_hash;
                    size_t buffer_cap = buffer_cap_attribs[buffer_cap_index].value;
                    Hof_Func hof_func = hof_func_attribs[hof_func_index].hof_func;
                    bench_file(file_path, expected_hash, buffer, buffer_cap, hof_func);
                }
                end_clock();
            }
            end_clock();
        }
    }
    end_clock();

    dump_summary(stderr);

    return 0;
}
