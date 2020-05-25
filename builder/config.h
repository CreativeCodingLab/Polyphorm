#include "microsoft_craziness.h"

namespace config
{
    // NEW ENVIRONMENTAL VARS
    char *env_var_new_info[3];
    const uint32_t env_var_new_info_count = ARRAYSIZE(env_var_new_info);
    
    char *args_debug   = "--debug";
    char *args_release = "--release";
    char *args_clean   = "--clean";
    char *args_run     = "run";

    #define CONFIG_BIN_DIR_VAR "bin"

    // OPTIMIZATION - /EHsc /fp:fast
    // MISC - MTd (static) Zc:inline?
    char *compiler_path = NULL;
    char *compiler_options_debug   = "/nologo /W3 /Od /RTC1 /MDd /EHsc /Zc:inline /Gm- /MP /Fo:" CONFIG_BIN_DIR_VAR "\\obj\\ /Fd" CONFIG_BIN_DIR_VAR "\\ /Zi /DDEBUG /link /INCREMENTAL:NO /subsystem:console /MACHINE:X64 /debug:fastlink /CGTHREADS:4";
    char *compiler_options_release = "/nologo /W3 /O2 /MD /EHsc /Zc:inline /Gm- /MP /Fo:" CONFIG_BIN_DIR_VAR "\\obj\\ /link /INCREMENTAL:NO /subsystem:console /MACHINE:X64 /debug:none /CGTHREADS:4";
    char *exe_file_prefix          = " /OUT:" CONFIG_BIN_DIR_VAR "\\";

    void setup_paths() {
        Find_Result result = find_visual_studio_and_windows_sdk();

        // Set CL.exe path.
        wchar_t *compiler_path_new = concat(result.vs_exe_path, L"\\cl.exe"); defer {free(compiler_path_new);};
        int compiler_path_length = wcslen(compiler_path_new) + 1;
        compiler_path = (char *)malloc(compiler_path_length);
        wcstombs(compiler_path, compiler_path_new, compiler_path_length);

        // Set LIBPATH variable.
        wchar_t *libpath_new = concat(L"LIBPATH=", result.vs_library_path); defer {free(libpath_new);};
        int libpath_length = wcslen(libpath_new) + 1;
        char *libpath = (char *)malloc(libpath_length);
        wcstombs(libpath, libpath_new, libpath_length);

        // Set LIB variable.
        wchar_t *lib_new1 = concat(L"LIB=", result.windows_sdk_ucrt_library_path, L";"); defer {free(lib_new1);};
        wchar_t *lib_new2 = concat(lib_new1, result.windows_sdk_um_library_path, L";"); defer {free(lib_new2);};
        wchar_t *lib_new3 = concat(lib_new2, result.vs_library_path, L";"); defer {free(lib_new3);};
        int lib_length = wcslen(lib_new3) + 1;
        char *lib = (char *)malloc(lib_length);
        wcstombs(lib, lib_new3, lib_length);

        // Set INCLUDE variable.
        wchar_t *include_new1 = concat(L"INCLUDE=", result.vs_include_path, L";"); defer {free(include_new1);};
        wchar_t *include_new2 = concat(include_new1, result.windows_sdk_um_include_path, L";"); defer {free(include_new2);};
        wchar_t *include_new3 = concat(include_new2, result.windows_sdk_ucrt_include_path, L";"); defer {free(include_new3);};
        wchar_t *include_new4 = concat(include_new3, result.windows_sdk_shared_include_path, L";"); defer {free(include_new4);};
        int include_length = wcslen(include_new4) + 1;
        char *include = (char *)malloc(include_length);
        wcstombs(include, include_new4, include_length);

        // Update array with new env variables.
        env_var_new_info[0] = include;
        env_var_new_info[1] = libpath;
        env_var_new_info[2] = lib;
        
        // Clean up.
        free_resources(&result);
    };
}
