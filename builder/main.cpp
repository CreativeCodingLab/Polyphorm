#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include "structs.h"
#include "config.h"
#include "memory.cpp"
#include "string.cpp"
#include "sys_utils.cpp"
#include "file_system.cpp"
#include "parser.cpp"
#include "hasher.cpp"
#include "environment.cpp"


int main(int argc, char **argv)
{
    // Check if no. arguments correct
    if (argc < 2) {
        printf("You need to provide a path to a build file.\n");
        return -1;
    }

    // Check if `run` subcommand specified
    bool is_run = false;
    if(string::equals(argv[1], config::args_run)) {
        is_run = true;
        if (argc < 3) {
            printf("You need to provide a path to a build file.\n");
            return -1;
        }
    }

    config::setup_paths();

    // Default settings
    bool arg_clean = false;
    bool arg_debug = true;

    // Parse args
    uint32_t optional_args_start_index = is_run ? 3 : 2;
    for (uint32_t i = optional_args_start_index; i < (uint32_t) argc; ++i)
    {
        char *option = argv[i];
        if (string::equals(option, config::args_debug))
        {
            arg_debug = true;
        }
        else if (string::equals(option, config::args_release))
        {
            arg_debug = false;
        }
        else if (string::equals(option, config::args_clean))
        {
            arg_clean = true;
        }
        else 
        {
            printf("Unrecognized argument %s.", option);
        }
    }
    
    Timer perf_timer = timer::get();
    timer::start(&perf_timer);
    memory::init();

    char *path_to_config_file   = is_run ? argv[2] : argv[1];
    auto  mem_state_config_file = memory::set(TEMPORARY);

    // Open config file
    File config_file = file_system::read_file(path_to_config_file, TEMPORARY);
    if (config_file.size == 0)
    {
        printf("Incorrect path to build file!\n");
        return -1;
    }

    // Set current dir to that of a config file
    {
        char *directory_path = file_system::get_directory_from_path(path_to_config_file, TEMPORARY);
        if (directory_path)
        {
            SetCurrentDirectoryA(directory_path);
        }
    }

    // In case cleaning option is specified, delete directory with a binary
    if (arg_clean)
    {
        printf("Cleaning bin folder.\n");
        FileList *files = file_system::find_files(CONFIG_BIN_DIR_VAR "/*");
        while(files)
        {
            file_system::delete_file_recursive(files->file_path);
            files = files->next_file;
        }

        file_system::delete_file(CONFIG_BIN_DIR_VAR);
        return 0;
    }

    // Parse config file
    BuildSettings build_settings = parse_config_file(config_file);
    memory::reset(mem_state_config_file, TEMPORARY);
    if(!build_settings.source_files || !build_settings.target_exe_name)
    {
        printf("Need to specify at least full 'build_exe' command - build_exe($exe_name, $source_files)\n");
        return -1;
    }

    // Create necessary directories
    {
        file_system::create_directory(CONFIG_BIN_DIR_VAR);
        file_system::create_directory(CONFIG_BIN_DIR_VAR "\\obj");
    }

    // Pre hash files
    {
        FileList *pre_hash_files = build_settings.pre_hash_files;
        if(pre_hash_files)
        {
            HashDB hash_db = {};
            auto mem_state_date = memory::set(TEMPORARY);
            char *backup_dir_path = sys_utils::get_current_time_and_date_string("'" CONFIG_BIN_DIR_VAR "\\pre_hash_'yy'_'MM'_'dd'_'",
                                                                                    "HH'_'mm'_'ss\\", true, TEMPORARY);
            file_system::create_directory(backup_dir_path);

            while(pre_hash_files)
            {
                hash_file(pre_hash_files->file_path, &hash_db, backup_dir_path);
                pre_hash_files = pre_hash_files->next_file;
            }
            memory::reset(mem_state_date, TEMPORARY);
        }
    }

    // Copy over specified files
    {
        CopyFileBatch *copy_file_batch = build_settings.copy_files;
        while(copy_file_batch)
        {
            FileList *copy_files = copy_file_batch->source_files;
            while(copy_files)
            {
                auto  mem_state = memory::set(TEMPORARY);
                char *copy_file_name = file_system::get_file_name_from_path(copy_files->file_path);
                char *target_copy_file_path = file_system::get_combined_paths(copy_file_batch->target_directory, copy_file_name, TEMPORARY);

                file_system::copy_file(copy_files->file_path, target_copy_file_path);
                memory::reset(mem_state, TEMPORARY);
                
                copy_files = copy_files->next_file;
            }

            copy_file_batch = copy_file_batch->next_batch;
        }
    }

    // Compose compile cmd line
    char *compile_cmd = NULL;
    {
        FileList *source_files = build_settings.source_files;
        uint32_t source_files_paths_length = 1; // Add a space after compiler path
        while(source_files)
        {
            source_files_paths_length += string::length(source_files->file_path) + 1;
            source_files = source_files->next_file;
        }

        FileList *include_dirs = build_settings.include_directories;
        // TODO: Check if the +1 is necesssary because of spaces being added for each path
        uint32_t include_dirs_path_length = 1; 
        while(include_dirs)
        {
            include_dirs_path_length += string::length(include_dirs->file_path) + 1 + 3; // Add a space before + '/I '
            include_dirs = include_dirs->next_file;
        }

        FileList *libraries = build_settings.libraries;
        // TODO: Check if the +1 is necesssary because of spaces being added for each path
        uint32_t libraries_length = 1; // Add a space before
        while(libraries)
        {
            libraries_length += string::length(libraries->file_path) + 1;
            libraries = libraries->next_file;
        }

        char *compiler_path    = config::compiler_path;
        char *compiler_options = arg_debug ? config::compiler_options_debug : config::compiler_options_release;

        uint32_t compiler_path_length    = string::length(compiler_path);
        uint32_t compiler_options_length = string::length(compiler_options);
        uint32_t exe_file_name_length    = string::length(build_settings.target_exe_name) + 1; // ntc
        uint32_t exe_file_prefix_length  = string::length(config::exe_file_prefix);
        uint32_t command_line_length     = compiler_path_length + source_files_paths_length + include_dirs_path_length +        
                                           compiler_options_length + exe_file_prefix_length + exe_file_name_length + libraries_length;
                                           
        compile_cmd = memory::allocate<char>(command_line_length, PERSISTENT);
        char *compile_cmd_ptr = compile_cmd;
        
        memcpy(compile_cmd_ptr, compiler_path, compiler_path_length);
        compile_cmd_ptr += compiler_path_length;
        *(compile_cmd_ptr++) = ' ';

        source_files = build_settings.source_files;
        while(source_files)
        {
            uint32_t file_path_length = string::length(source_files->file_path);
            memcpy(compile_cmd_ptr, source_files->file_path, file_path_length);
            compile_cmd_ptr += file_path_length;
            *(compile_cmd_ptr++) = ' ';
            source_files = source_files->next_file;
        }

        include_dirs = build_settings.include_directories;
        
        while(include_dirs)
        {
            memcpy(compile_cmd_ptr, "/I ", 3);
            compile_cmd_ptr += 3;

            uint32_t include_dir_length = string::length(include_dirs->file_path);
            memcpy(compile_cmd_ptr, include_dirs->file_path, include_dir_length);
            compile_cmd_ptr += include_dir_length;
            *(compile_cmd_ptr++) = ' ';
            include_dirs = include_dirs->next_file;
        }

        memcpy(compile_cmd_ptr, compiler_options, compiler_options_length);
        compile_cmd_ptr += compiler_options_length;
        *(compile_cmd_ptr++) = ' ';
        
        
        libraries = build_settings.libraries;
        while(libraries)
        {
            uint32_t library_length = string::length(libraries->file_path);
            memcpy(compile_cmd_ptr, libraries->file_path, library_length);
            compile_cmd_ptr += library_length;
            *(compile_cmd_ptr++) = ' ';
            libraries = libraries->next_file;
        }

        memcpy(compile_cmd_ptr, config::exe_file_prefix, exe_file_prefix_length);
        compile_cmd_ptr += exe_file_prefix_length;
        memcpy(compile_cmd_ptr, build_settings.target_exe_name, exe_file_name_length - 1);
        compile_cmd_ptr += exe_file_name_length;
        *(compile_cmd_ptr - 1) = 0;
    }
    
    // Get updated environment settings
    char *new_env_block = NULL;
    {
        char **env_var_new_info                = config::env_var_new_info;
        uint32_t env_var_new_info_count        = config::env_var_new_info_count;

        char *old_env_block = environment::get_current_env_block();
        new_env_block = environment::update_env_block(old_env_block, env_var_new_info, env_var_new_info_count,
                                                      PERSISTENT);
        environment::free_env_block(old_env_block);
    }
    
    // Create compilation process
    printf("Source files:\n");
    {
        PROCESS_INFORMATION process_info = {};
        STARTUPINFO         startup_info = {};
        startup_info.cb = sizeof(startup_info);
        printf("%s\n", compile_cmd);

        BOOL return_status = CreateProcessA(NULL, compile_cmd, NULL, NULL, TRUE, NULL, new_env_block, NULL, &startup_info, &process_info);
        if (!return_status)
        {
            char *messageBuffer;
            DWORD  error = GetLastError();
            size_t size  = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
            printf("Call to compiler failed! %s\n", messageBuffer);
            return -1;
        }   

        WaitForSingleObject(process_info.hProcess, 5000);
        DWORD exit_code;
        GetExitCodeProcess(process_info.hProcess, &exit_code);
        if(exit_code)
        {
            printf("Compilation failed! (%fs)\n", timer::end(&perf_timer));
            return -1;
        }
        else
        {
            printf("Compilation done! (%fs)\n", timer::end(&perf_timer));
            printf("> %s\n", build_settings.target_exe_name);
        }
    }

    if (is_run) {
        char *config_directory_path = file_system::get_directory_from_path(path_to_config_file, TEMPORARY);

        // +2 for backslashes before and after bin directory
        // +1 for ntc
        uint32_t config_directory_path_length  = string::length(config_directory_path);
        uint32_t bin_dir_length                = string::length(CONFIG_BIN_DIR_VAR);
        uint32_t exe_file_length               = string::length(build_settings.target_exe_name);
        uint32_t working_directory_path_length = config_directory_path_length + bin_dir_length + 3;
        uint32_t run_cmd_length                = config_directory_path_length + bin_dir_length + exe_file_length + 3;

        // Compose working directory path
        char *working_directory_path = memory::allocate<char>(working_directory_path_length, PERSISTENT);
        char *working_directory_path_ptr = working_directory_path;
        
        memcpy(working_directory_path_ptr, config_directory_path, config_directory_path_length);
        working_directory_path_ptr += config_directory_path_length;

        memcpy(working_directory_path_ptr, "\\" CONFIG_BIN_DIR_VAR "\\", bin_dir_length + 2);
        working_directory_path_ptr += bin_dir_length + 2;
        *working_directory_path_ptr = 0;

        // Compose run command
        char *run_cmd = memory::allocate<char>(run_cmd_length, PERSISTENT);
        char *run_cmd_ptr = run_cmd;
        
        memcpy(run_cmd_ptr, config_directory_path, config_directory_path_length);
        run_cmd_ptr += config_directory_path_length;

        memcpy(run_cmd_ptr, "\\" CONFIG_BIN_DIR_VAR "\\", bin_dir_length + 2);
        run_cmd_ptr += bin_dir_length + 2;

        memcpy(run_cmd_ptr, build_settings.target_exe_name, exe_file_length);
        run_cmd_ptr += exe_file_length;
        *run_cmd_ptr = 0;

        // Run
        PROCESS_INFORMATION process_info = {};
        STARTUPINFO         startup_info = {};
        startup_info.cb = sizeof(startup_info);
        BOOL return_status = CreateProcessA(NULL, run_cmd, NULL, NULL, TRUE, NULL, new_env_block, working_directory_path, &startup_info, &process_info);
        if (!return_status)
        {
            char *messageBuffer;
            DWORD  error = GetLastError();
            size_t size  = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
            printf("Running %s failed! %s\n", build_settings.target_exe_name, messageBuffer);
            return -1;
        }   

        WaitForSingleObject(process_info.hProcess, 5000);
        DWORD exit_code;
        GetExitCodeProcess(process_info.hProcess, &exit_code);
    }

    return 0;
}