#include "file_system.h"
#include <windows.h>

#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif


File file_system::read_file(char *path) {
    File file = {};

    // Open handle to a file
    HANDLE file_handle = CreateFileA(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        PRINT_DEBUG("Unable to open read handle to file %s.", path);
        return file;
    }

    // Get file attributes, these are necessary to know how much data to allocate for reading
    WIN32_FILE_ATTRIBUTE_DATA file_attributes;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &file_attributes)) {
        PRINT_DEBUG("Unable to get attributes of file %s.", path);
        CloseHandle(file_handle);
        return File{};
    }

    // Allocate memory for reading the file contents
    uint32_t file_size = file_attributes.nFileSizeLow;
    HANDLE heap = GetProcessHeap();
    file.data = HeapAlloc(heap, HEAP_ZERO_MEMORY, file_size);

    // Read the file into allocated memory
    DWORD bytes_read_from_file = 0;
    if (ReadFile(file_handle, file.data, file_size, &bytes_read_from_file, NULL) &&
        bytes_read_from_file == file_size) {
        file.size = bytes_read_from_file;
    } else {
        // In case of read error, deallocate memory and close handle
        PRINT_DEBUG("Unable to read opened file %s.", path);
        HeapFree(heap, 0, file.data);
        CloseHandle(file_handle);
        return File{};
    }

    // Succesfull file reading, close the handle and return the data
    CloseHandle(file_handle);
    return file;
}

void file_system::release_file(File file) {
    // Release allocated memory from the heap
    HANDLE heap = GetProcessHeap();
    HeapFree(heap, 0, file.data);
}

uint32_t file_system::write_file(char *path, void *data, uint32_t size) {
    // Open handle to a file
    HANDLE file_handle = 0;
    file_handle = CreateFileA(
        path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (file_handle == INVALID_HANDLE_VALUE) {
        PRINT_DEBUG("Unable to open write handle to file %s.", path);
        return 0;
    }

    // Write to a file
    DWORD bytes_written = 0;
    if(!WriteFile(file_handle, data, size, &bytes_written, NULL)) {
        PRINT_DEBUG("Unable to write to file %s.", path);
        return 0;
    }

    // Wrote sucessfully, close the handle
    CloseHandle(file_handle);
    return bytes_written;
}

void file_system::mkdir(char *path) {
    int result = CreateDirectoryA(path, NULL);
    if(result == 0) {
        PRINT_DEBUG("Unable to create directory %s.", path);
    }
}

FILETIME file_system::get_last_write_time(char *file_path) {
	FILETIME write_time = {};
	WIN32_FILE_ATTRIBUTE_DATA file_data;
	if (GetFileAttributesExA(file_path, GetFileExInfoStandard, &file_data)) {
		write_time = file_data.ftLastWriteTime;
	}
	return write_time;
}