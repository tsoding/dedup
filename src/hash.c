#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "./hash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

void hash_of_file_mmap(const char *file_path, BYTE *buffer_ignored, size_t buffer_cap_ignored, Hash *hash)
{
    (void) buffer_ignored;
    (void) buffer_cap_ignored;

    SHA256_CTX ctx;
    memset(&ctx, 0, sizeof(ctx));
    sha256_init(&ctx);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Could not open file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(statbuf));
    if (fstat(fd, &statbuf) < 0) {
        fprintf(stderr, "ERROR: Could not determine the size of the file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    size_t buffer_size = statbuf.st_size;

    void *buffer = mmap(NULL, buffer_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: could not memory map file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    sha256_update(&ctx, buffer, buffer_size);

    sha256_final(&ctx, hash->bytes);

    munmap(buffer, buffer_size);
    close(fd);
}

void hash_of_file_linux(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash)
{
    SHA256_CTX ctx;
    memset(&ctx, 0, sizeof(ctx));
    sha256_init(&ctx);

    int f = open(file_path, O_RDONLY);
    if (f < 0) {
        fprintf(stderr, "ERROR: Could not open file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    int buffer_size = read(f, buffer, buffer_cap);
    while (buffer_size > 0) {
        sha256_update(&ctx, buffer, buffer_size);
        buffer_size = read(f, buffer, buffer_cap);
    }

    if (buffer_size < 0) {
        fprintf(stderr, "ERROR: Could not read from file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    close(f);

    sha256_final(&ctx, hash->bytes);
}

// TODO: speed up hash_of_file function
void hash_of_file_libc(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash)
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

    size_t buffer_size = fread(buffer, 1, buffer_cap, f);
    while (buffer_size > 0) {
        sha256_update(&ctx, buffer, buffer_size);
        buffer_size = fread(buffer, 1, buffer_cap, f);
    }

    if (ferror(f)) {
        fprintf(stderr, "Could not read from file %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);

    sha256_final(&ctx, hash->bytes);
}

static char hex_digit(unsigned int digit)
{
    digit = digit % 0x10;
    if (digit <= 9) return digit + '0';
    if (10 <= digit && digit <= 15) return digit - 10 + 'a';
    assert(0 && "unreachable");
}

void hash_as_cstr(Hash input, char output[32*2 + 1])
{
    for (size_t i = 0; i < 32; ++i) {
        output[i*2 + 0] = hex_digit(input.bytes[i] / 0x10);
        output[i*2 + 1] = hex_digit(input.bytes[i]);
    }
    output[32*2] = '\0';
}

static bool parse_hexdigit(char c, uint8_t *digit)
{
    if ('0' <= c && c <= '9') *digit = c - '0';
    else if ('a' <= c && c <= 'f') *digit = c - 'a' + 10;
    else if ('A' <= c && c <= 'F') *digit = c - 'a' + 10;
    else return false;
    return true;
}

bool parse_hash(const char input[64], Hash *output)
{
    for (size_t i = 0; i < 32; ++i) {
        uint8_t a, b;
        if (!parse_hexdigit(input[2*i + 0], &a)) return false;
        if (!parse_hexdigit(input[2*i + 1], &b)) return false;
        output->bytes[i] = a * 0x10 + b;
    }
    return true;
}
