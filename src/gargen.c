#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FLAG_IMPLEMENTATION
#include "./flag.h"

// NOTE: Stolen from https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/
uint64_t lehmer64()
{
    static __uint128_t g_lehmer64_state = 0xda942042e4dd58b5;
    g_lehmer64_state *= 0xda942042e4dd58b5;
    return g_lehmer64_state >> 64;
}

void fill_buffer_with_garbage(uint8_t *buffer, size_t buffer_size)
{
    uint64_t *buffer_u64 = (uint64_t*) buffer;
    size_t buffer_u64_size = buffer_size / 8;

    for (size_t i = 0; i < buffer_u64_size; ++i) {
        buffer_u64[i] = lehmer64();
    }

    for (size_t i = 0; i < buffer_size % 8; ++i) {
        buffer[buffer_u64_size*8 + i] = (uint8_t) lehmer64();
    }
}

void usage(FILE *stream)
{
    fprintf(stream, "Usage: gargen [OPTIONS] <FILE>\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv)
{
    uint64_t *file_size = flag_uint64("file-size", 1024, "The size of the file to generate in bytes");
    uint64_t *buffer_size = flag_uint64("buffer-size", 1024, "The size of the buffer to use during the generation process");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (flag_rest_argc() < 1) {
        usage(stderr);
        fprintf(stderr, "ERROR: no output file path is provided\n");
        exit(1);
    }

    if (*buffer_size == 0) {
        usage(stderr);
        fprintf(stderr, "ERROR: buffer size may not be equal to 0\n");
        exit(1);
    }

    const char *file_path = flag_rest_argv()[0];

    printf("File Path:   %s\n", file_path);
    printf("File Size:   %"PRIu64"\n", *file_size);
    printf("Buffer Size: %"PRIu64"\n", *buffer_size);

    uint8_t *buffer = malloc(*buffer_size);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: could not allocate memory for the buffer: %s\n",
                strerror(errno));
        exit(1);
    }

    FILE *fout = fopen(file_path, "wb");
    if (fout == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    while (*file_size > 0) {
        size_t chunk_size = *buffer_size;
        if (chunk_size > *file_size) {
            chunk_size = *file_size;
        }

        fill_buffer_with_garbage(buffer, chunk_size);
        fwrite(buffer, chunk_size, 1, fout);
        if (ferror(fout)) {
            fprintf(stderr, "ERROR: could not write to file %s: %s\n",
                    file_path, strerror(errno));
            exit(1);
        }

        *file_size -= chunk_size;
    }
    fclose(fout);

    return 0;
}
