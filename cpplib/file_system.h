#pragma once
#include <stdint.h>

// File encapsulates data loaded from the file and its size.
struct File
{
    void *data;
    uint32_t size;
};

// file_system namespace enables interfacing with the Windows file system
namespace file_system
{
    // Read file at path
    File read_file(const char* path);

    // Release read file
    void release_file(File file);

    // Write data to file at path
    uint32_t write_file(const char* path, void *data, uint32_t size);
}

