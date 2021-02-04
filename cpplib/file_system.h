#pragma once
#include <stdint.h>
#include <windows.h>

// File encapsulates data loaded from the file and its size.
struct File {
    void *data;
    uint32_t size;
};

// file_system namespace enables interfacing with the Windows file system
namespace file_system {
    // Read file at path
    File read_file(char *path);

    // Release read file
    void release_file(File file);

    // Write data to file at path
    uint32_t write_file(char *path, void *data, uint32_t size);

    // Create empty directory
    void mkdir(char *path);

    // Check the last time file was written to.
    FILETIME get_last_write_time(char *file_path);
}

#ifdef CPPLIB_FILESYSTEM_IMPL
#include "file_system.cpp"
#endif