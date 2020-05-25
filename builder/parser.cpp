char *KEYWORD_BUILD     = "build_exe";
char *KEYWORD_COPY      = "copy";
char *KEYWORD_PREHASH   = "pre_hash";
char *KEYWORD_LIBRARIES = "libs";
char *KEYWORD_INCLUDE   = "include_dirs";

enum TokenTypes
{
    TOKEN_IDENTIFIER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_COMMA,
    TOKEN_EOF,
    TOKEN_VARIABLE
};

struct Token
{
    TokenTypes type;
    char *data;
    uint32_t data_size;
};

struct Lexer
{
    char *current_ptr;
    char *base_ptr;
    uint32_t total_size;
    uint32_t current_line_number = 0;
};

bool is_alpha(char c)
{
    bool result = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    return result;
}

bool is_num(char c)
{
    bool result = (c >= '0') && (c <= '9');
    return result;
}

bool token_string_equal(Token token, char *string)
{
    uint32_t i = 0;
    for(; i < token.data_size; ++i)
    {
        if (token.data[i] != string[i]) return false;
    }

    if (string[i + 1] != 0) return false;
    return true;
}

char *make_string_from_token(Token token, AllocatorType allocator_type = PERSISTENT)
{
    char *string_storage = string::create_null_terminated(token.data, token.data_size, allocator_type);
    return string_storage;
}

char *get_value_from_var(Token token)
{
    if(token.type != TOKEN_VARIABLE) return NULL;

    if(string::equals(token.data, "$BIN", token.data_size))
    {
        return CONFIG_BIN_DIR_VAR;
    }

    return NULL;
}

Token get_next_token(Lexer *lexer)
{
    while(true)
    {
        if(lexer->current_ptr - lexer->base_ptr >= lexer->total_size)
        {
            Token result = { TOKEN_EOF, 0, 0 };
            return result;
        }

        char current_char = *(lexer->current_ptr++);
        if (current_char == '(')
        {
            Token result = { TOKEN_LEFT_PAREN, 0, 0 };
            return result;
        }
        else if (current_char == ')')
        {
            Token result = { TOKEN_RIGHT_PAREN, 0, 0 };
            return result;
        }
        else if (current_char == ',')
        {
            Token result = { TOKEN_COMMA, 0, 0 };
            return result;
        }
        else if (is_alpha(current_char) || current_char == '/' ||current_char == '.')
        {
            Token result = { TOKEN_IDENTIFIER, lexer->current_ptr - 1, 1 };
            current_char = *lexer->current_ptr;
            while(is_alpha(current_char) || is_num(current_char) ||
                  current_char == '.' || current_char == '_' || 
                  current_char == '/' || current_char == '*' ||
                  current_char == '-')
            {
                current_char = *(++lexer->current_ptr);
                result.data_size++;
            }
            return result;
        }
        else if (current_char == '$')
        {
            Token result = { TOKEN_VARIABLE, lexer->current_ptr - 1, 1 };
            current_char = *lexer->current_ptr;
            while(is_alpha(current_char) || is_num(current_char) ||
                  current_char == '_')
            {
                current_char = *(++lexer->current_ptr);
                result.data_size++;
            }
            return result;
        }
        else if (current_char == '\n')
        {
            lexer->current_line_number++;
        }
    }
}

FileList *parse_file_list(Token initial_token, Lexer *lexer, Token *last_token)
{
    Token next_token = initial_token;
    FileList *file_list = NULL, *current_file = NULL, *previous_file = NULL;
    while(next_token.type == TOKEN_IDENTIFIER)
    {
        // Cache previous file node
        if(current_file) previous_file = current_file;

        auto mem_state = memory::set(TEMPORARY);
        current_file = file_system::find_files(make_string_from_token(next_token, TEMPORARY));
        memory::reset(mem_state, TEMPORARY);

        // If there's a previous file node (so every iteration except for the first one),
        // let previous file node point to the new one
        if(previous_file) previous_file->next_file = current_file;
        
        // In case this is first file node, store pointer for later
        if(!file_list) file_list = current_file;

        // Walk to the end of the list
        while(current_file->next_file) current_file = current_file->next_file;

        // Continue
        next_token = get_next_token(lexer);
    }

    if (current_file) current_file->next_file = NULL;
    if (last_token) *last_token = next_token;

    return file_list;
}

FileList *parse_string_list(Token initial_token, Lexer *lexer, Token *last_token)
{
    Token next_token = initial_token;
    FileList *file_list = NULL, *current_file = NULL, *previous_file = NULL;
    while(next_token.type == TOKEN_IDENTIFIER)
    {
        // Cache previous file node
        if(current_file) previous_file = current_file;

        // Allocate and fill a new node
        current_file = memory::allocate<FileList>(1, PERSISTENT);
        current_file->file_path = make_string_from_token(next_token);
        
        // If there's a previous file node (so every iteration except for the first one),
        // let previous file node point to the new one
        if(previous_file) previous_file->next_file = current_file;
        
        // In case this is first file node, store pointer for later
        if(!file_list) file_list = current_file;

        // Continue
        next_token = get_next_token(lexer);
    }

    if (current_file) current_file->next_file = NULL;
    if (last_token) *last_token = next_token;

    return file_list;
}

bool parse_build_command(Lexer *lexer, BuildSettings *build_settings)
{
    Token next_token = get_next_token(lexer);
    if(next_token.type != TOKEN_LEFT_PAREN)
    {
        printf("Expected left paren after 'build_exe' command. Line %d.\n", lexer->current_line_number);
        return false;
    }

    next_token = get_next_token(lexer);
    if (next_token.type != TOKEN_IDENTIFIER)
    {
        printf("Expected name of the exe after paren in 'build_exe' command. Line %d.\n", lexer->current_line_number);
        return false;
    }
        
    Token target_exe_token = next_token;
    next_token = get_next_token(lexer);
    if (next_token.type != TOKEN_COMMA)
    {
        printf("Expected a comma after the exe name in 'build_exe' command. Line %d.\n", lexer->current_line_number);
        return false;
    }

    next_token = get_next_token(lexer);
    auto mem_state_persistent = memory::set(PERSISTENT);
    
    FileList *file_list = parse_file_list(next_token, lexer, &next_token);
    if (!file_list)
    {
        printf("Expected at least one source file in 'build_exe' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    if (next_token.type != TOKEN_RIGHT_PAREN)
    {
        printf("Expected right paren at the end of 'build_exe' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    build_settings->target_exe_name = make_string_from_token(target_exe_token);
    build_settings->source_files = file_list;
    return true;
}

bool parse_pre_hash_command(Lexer *lexer, BuildSettings *build_settings)
{
    Token next_token = get_next_token(lexer);
    if(next_token.type != TOKEN_LEFT_PAREN)
    {
        printf("Expected left paren after 'pre_hash' command. Line %d.\n", lexer->current_line_number);
        return false;
    }
    
    next_token = get_next_token(lexer);
    auto mem_state_persistent = memory::set(PERSISTENT);
    
    FileList *file_list = parse_file_list(next_token, lexer, &next_token);
    if (!file_list)
    {
        printf("Expected at least one source file in 'pre_hash' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    if (next_token.type != TOKEN_RIGHT_PAREN)
    {
        printf("Expected right paren at the end of 'pre_hash' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }
    
    build_settings->pre_hash_files = file_list;
    return true;
}


bool parse_copy_command(Lexer *lexer, BuildSettings *build_settings)
{
    Token next_token = get_next_token(lexer);
    if(next_token.type != TOKEN_LEFT_PAREN)
    {
        printf("Expected left paren after 'copy' command. Line %d.\n", lexer->current_line_number);
        return false;
    }

    next_token = get_next_token(lexer);
    auto mem_state_persistent = memory::set(PERSISTENT);
    
    FileList *file_list = parse_file_list(next_token, lexer, &next_token);
    if (!file_list)
    {
        printf("Expected at least one source file in 'copy' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    if (next_token.type != TOKEN_COMMA)
    {
        printf("Expected a comma after the exe name in 'copy' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    next_token = get_next_token(lexer);
    if (next_token.type != TOKEN_IDENTIFIER && next_token.type != TOKEN_VARIABLE)
    {
        printf("Expected target directory after a comma in 'copy' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    auto mem_state = memory::set(TEMPORARY);
    char *copy_target_directory = NULL, *copy_target_directory_ptr = NULL;
    
    while(next_token.type == TOKEN_IDENTIFIER || next_token.type == TOKEN_VARIABLE)
    {
        uint32_t string_length = 0;
        char *source_string    = NULL;
        if(next_token.type == TOKEN_IDENTIFIER)
        {
            source_string = next_token.data;
            string_length = next_token.data_size;
        }
        else
        {
            source_string = get_value_from_var(next_token);
            if(!source_string)
            {
                printf("Unrecognized variable name %.*s. Line %d.\n", next_token.data_size, next_token.data, lexer->current_line_number);
                memory::reset(mem_state_persistent, PERSISTENT);
                memory::reset(mem_state, TEMPORARY);
                return false;
            }
            string_length = string::length(source_string);
        }

        // Since there's no allocation in the loop, we can just extend allocated memory block
        copy_target_directory_ptr = memory::allocate<char>(string_length, TEMPORARY);
        if(!copy_target_directory) copy_target_directory = copy_target_directory_ptr;
        memcpy(copy_target_directory_ptr, source_string, string_length);

        next_token = get_next_token(lexer);
    }

    copy_target_directory_ptr = memory::allocate<char>(1, TEMPORARY);
    *copy_target_directory_ptr = 0;

    char *copy_target_directory_absolute_path = file_system::get_absolute_path(copy_target_directory, PERSISTENT);
    memory::reset(mem_state, TEMPORARY);

    if (next_token.type != TOKEN_RIGHT_PAREN)
    {
        printf("Expected right paren at the end of 'copy' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    CopyFileBatch *next_copy_batch = NULL;
    if (!build_settings->copy_files)
    {
        // In case of first copy batch, allocate initial batch and use it
        build_settings->copy_files = memory::allocate<CopyFileBatch>(1, PERSISTENT);
        next_copy_batch = build_settings->copy_files;
    } 
    else
    {
        // In case of additional copy batches, find the last item in list, add a new one and use it
        next_copy_batch = build_settings->copy_files;
        while(next_copy_batch->next_batch)
        {
            next_copy_batch = next_copy_batch->next_batch;
        }
        next_copy_batch->next_batch = memory::allocate<CopyFileBatch>(1, PERSISTENT);
        next_copy_batch = next_copy_batch->next_batch;
    }

    next_copy_batch->next_batch = NULL;
    next_copy_batch->target_directory = copy_target_directory_absolute_path;
    next_copy_batch->source_files = file_list; 
    return true;
}

bool parse_libraries_command(Lexer *lexer, BuildSettings *build_settings)
{
    Token next_token = get_next_token(lexer);
    if(next_token.type != TOKEN_LEFT_PAREN)
    {
        printf("Expected left paren after 'libraries' command. Line %d.\n", lexer->current_line_number);
        return false;
    }

    next_token = get_next_token(lexer);
    auto mem_state_persistent = memory::set(PERSISTENT);
    
    FileList *file_list = parse_string_list(next_token, lexer, &next_token);
    if(!file_list)
    {
        printf("Expected at least one source file in 'libraries' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    if (next_token.type != TOKEN_RIGHT_PAREN)
    {
        printf("Expected right paren at the end of 'libraries' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    build_settings->libraries = file_list;
    return true;
}

bool parse_include_command(Lexer *lexer, BuildSettings *build_settings)
{
    Token next_token = get_next_token(lexer);
    if(next_token.type != TOKEN_LEFT_PAREN)
    {
        printf("Expected left paren after 'include_dir' command. Line %d.\n", lexer->current_line_number);
        return false;
    }
    
    next_token = get_next_token(lexer);
    auto mem_state_persistent = memory::set(PERSISTENT);
    
    FileList *file_list = parse_file_list(next_token, lexer, &next_token);
    if (!file_list)
    {
        printf("Expected at least one directory in 'include_dir' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }

    if (next_token.type != TOKEN_RIGHT_PAREN)
    {
        printf("Expected right paren at the end of 'include_dir' command. Line %d.\n", lexer->current_line_number);
        memory::reset(mem_state_persistent, PERSISTENT);
        return false;
    }
    
    if (!build_settings->include_directories)
    {
        build_settings->include_directories = file_list;
    }
    else
    {
        FileList *current_include_directory = build_settings->include_directories;
        while(current_include_directory->next_file != NULL) {
            current_include_directory = current_include_directory->next_file;
        }
        current_include_directory->next_file = file_list;
    }
    return true;
}

BuildSettings parse_config_file(File config_file)
{
    BuildSettings build_settings = {};
    Lexer lexer = { 
        (char *)config_file.data, 
        (char *)config_file.data, 
        config_file.size, 0
    };

    while(true)
    {
        Token current_token = get_next_token(&lexer);
        if (current_token.type == TOKEN_IDENTIFIER)
        {
            if (token_string_equal(current_token, KEYWORD_BUILD))
                parse_build_command(&lexer, &build_settings);
            else if (token_string_equal(current_token, KEYWORD_COPY))
                parse_copy_command(&lexer, &build_settings);
            else if (token_string_equal(current_token, KEYWORD_LIBRARIES))
                parse_libraries_command(&lexer, &build_settings);
            else if (token_string_equal(current_token, KEYWORD_PREHASH))
                parse_pre_hash_command(&lexer, &build_settings);
            else if (token_string_equal(current_token, KEYWORD_INCLUDE))
                parse_include_command(&lexer, &build_settings);
            else
                printf("Uncrecognized identifier '%.*s'.\n", current_token.data_size, current_token.data);
        }
        else if (current_token.type == TOKEN_EOF)
        {
            break;
        }
    }

    return build_settings;
}