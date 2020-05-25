struct File
{
    void *data;
    uint32_t size;
};

namespace file_system
{
    char *get_absolute_path(char *relative_path, AllocatorType allocator_type = PERSISTENT)
    {
        uint32_t needed_buffer_size = GetFullPathNameA(relative_path, 0, NULL, NULL);
        char *path_buffer = memory::allocate<char>(needed_buffer_size, allocator_type);   
        uint32_t path_length = GetFullPathNameA(relative_path, needed_buffer_size, path_buffer, NULL);
        return path_buffer;
    }

    char *get_file_name_from_path(char *path)
    {
        int32_t path_length = (int32_t) string::length(path);
        for(int32_t i = path_length - 1; i >= 0; --i)
        {
            if (path[i] == '/' || path[i] == '\\') return path + i + 1;
        }
        return path;
    }

    char *get_directory_from_path(char *path, AllocatorType allocator_type = PERSISTENT)
    {
        char *directory_path = NULL;
        char *file_start     = file_system::get_file_name_from_path(path);
        uint32_t directory_path_length = (uint32_t) (file_start - path);
        if(directory_path_length)
        {
            directory_path = string::create_null_terminated(path, directory_path_length, allocator_type);
        }
        return directory_path;
    }

    char *get_combined_paths(char *path_first, char *path_second, AllocatorType allocator_type = PERSISTENT)
    {
        char *combined_path = memory::allocate<char>(MAX_PATH, allocator_type);
        PathCombineA(combined_path, path_first, path_second);
        return combined_path;
    }
    
    File read_file(char *path, AllocatorType allocator_type = PERSISTENT)
    {
        File file = {};

        HANDLE file_handle = CreateFileA(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            printf("File %s not found!\n", path);
            return file;
        }

        WIN32_FILE_ATTRIBUTE_DATA file_attributes;
        if (!GetFileAttributesExA(path, GetFileExInfoStandard, &file_attributes))
        {
            printf("Failed to get file attributes for file %s!\n", path);
            CloseHandle(file_handle);
            return file;
        }

        uint32_t file_size = file_attributes.nFileSizeLow;
        auto mem_state = memory::set(allocator_type);
        file.data = memory::allocate<char>(file_size, allocator_type);
        DWORD bytes_read_from_file = 0;
        if (ReadFile(file_handle, file.data, file_size, &bytes_read_from_file, NULL) &&
            bytes_read_from_file == file_size)
        {
            file.size = bytes_read_from_file;
        }
        else
        {
            memory::reset(mem_state, allocator_type);
            CloseHandle(file_handle);
            printf("Failed to read file %s!\n", path);
            return file;
        }

        CloseHandle(file_handle);
        return file;
    }

    uint32_t write_file(char *path, void *data, uint32_t size)
    {
        HANDLE file_handle = 0;
        file_handle = CreateFileA(path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            printf("Error opening file %s for writing!\n", path);
            return 0;
        }

        DWORD bytes_written = 0;
        if(!WriteFile(file_handle, data, size, &bytes_written, NULL))
        {
            printf("Error writing to file %s!\n", path);
        }
        
        CloseHandle(file_handle);
        return bytes_written;
    }

    void create_directory(char *dir_path)
    {
        CreateDirectoryA(dir_path, NULL);
    }
    
    void copy_file(char *source_path, char *target_path)
    {
        // Iteratively try to create all the directories in the path
        uint32_t target_path_length = string::length(target_path);
        for (uint32_t i = 0; i < target_path_length; ++i)
        {
            if (target_path[i] == '/' || target_path[i] == '\\')
            {
                auto mem_state = memory::set(TEMPORARY);
                char *dir_path = string::create_null_terminated(target_path, i, TEMPORARY);
                file_system::create_directory(dir_path);
                memory::reset(mem_state, TEMPORARY);
            }
        }
        CopyFile(source_path, target_path, FALSE);
    }

    FileList *find_files(char *file_pattern)
    {
        // Strip slashes from the end, if any - this is cause of the way FindFirstFile functions
        int32_t file_pattern_length = (int32_t)string::length(file_pattern);
        for(int32_t i = file_pattern_length - 1; i > 0; ++i)
        {
            if(file_pattern[i] == '/' || file_pattern[i] == '\\')
            {
                file_pattern[i] = 0;
            }
            else
            {
                break;
            }
        }
        
        auto pre_dir_mem_state = memory::set(TEMPORARY);
        char *dir_path = file_system::get_directory_from_path(file_pattern, TEMPORARY);

        // Find all the files and store in a list
        WIN32_FIND_DATA bin_file_data;
        HANDLE bin_file = FindFirstFileA(file_pattern, &bin_file_data);
        FileList *file_list = NULL, *current_file = NULL, *previous_file = NULL;
        if (bin_file != INVALID_HANDLE_VALUE)
        {
            BOOL next_file_found = FALSE;
            do
            {
                // Concatenate found path with a directory
                auto mem_state = memory::set(TEMPORARY);
                char *file_path          = file_system::get_combined_paths(dir_path, bin_file_data.cFileName, TEMPORARY);
                char *absolute_file_path = file_system::get_absolute_path(file_path, PERSISTENT);
                memory::reset(mem_state, TEMPORARY);

                if(string::equals(bin_file_data.cFileName, "..") ||
                   string::equals(bin_file_data.cFileName, "."))
                {
                    // Next file
                    next_file_found = FindNextFileA(bin_file, &bin_file_data);
                    continue;
                }

                // Create a new FileList node
                if(current_file) previous_file = current_file;
                current_file = memory::allocate<FileList>(1, PERSISTENT);
                current_file->file_path = absolute_file_path;
                if(!file_list) file_list = current_file;
                if(previous_file) previous_file->next_file = current_file;

                // Next file
                next_file_found = FindNextFileA(bin_file, &bin_file_data);
            } while(next_file_found);

            if (current_file) current_file->next_file = NULL;
        }
        
        FindClose(bin_file);
        memory::reset(pre_dir_mem_state, TEMPORARY);
        return file_list;
    }

    void delete_file(char *file_path)
    {
        DWORD attributes = GetFileAttributes(file_path);
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectoryA(file_path);
        }
        else
        {
            DeleteFileA(file_path);
        }
    }

    void delete_file_recursive(char *file_path)
    {
        DWORD attributes = GetFileAttributes(file_path);
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // In case of a directory, add all its content to the list recursively
            auto mem_state = memory::set(PERSISTENT);
            char *dir_content_pattern = file_system::get_combined_paths(file_path, "*", PERSISTENT);
            FileList *dir_contents = file_system::find_files(dir_content_pattern);

            while(dir_contents) 
            {
                file_system::delete_file_recursive(dir_contents->file_path);
                dir_contents = dir_contents->next_file;
            }

            memory::reset(mem_state, PERSISTENT);
            RemoveDirectoryA(file_path);
        }
        else
        {
            DeleteFileA(file_path);
        }
    }
}
