#ifndef HASH_H_
#define HASH_H_

#include "./sha256.h"

typedef struct {
    BYTE bytes[32];
} Hash;

void hash_as_cstr(Hash hash, char output[32*2 + 1]);
void hash_of_file_libc(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash);
void hash_of_file_linux(const char *file_path, BYTE *buffer, size_t buffer_cap, Hash *hash);
void hash_of_file_mmap(const char *file_path, BYTE *buffer_ignored, size_t buffer_cap_ignored, Hash *hash);

#endif // HASH_H_
