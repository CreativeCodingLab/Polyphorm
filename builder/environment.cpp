uint32_t get_env_additional_info_length(char **env_additional_info, uint32_t env_additional_info_count)
{
    uint32_t env_additional_info_length = 0;

    for(uint32_t i = 0; i < env_additional_info_count; ++i)
    {
        env_additional_info_length += string::length(env_additional_info[i]);
    }
    
    return env_additional_info_length;
}

uint32_t get_env_new_info_length(char **env_new_info, uint32_t env_new_info_count)
{
    uint32_t env_new_info_length = 0;

    for(uint32_t i = 0; i < env_new_info_count; ++i)
    {
        env_new_info_length += string::length(env_new_info[i]) + 1;
    }
    
    return env_new_info_length;
}

uint32_t get_env_block_length(char *env_block)
{
    // Counting also the last NULL terminating character (There are two NTCs at the end of the env block)
    uint32_t env_block_length = 1;
    while(true)
    {
        uint32_t env_var_length = string::length(env_block) + 1;
        env_block_length += env_var_length;
        env_block        += env_var_length;
        if (*env_block == 0) break;
    }
    return env_block_length;
}

uint32_t get_var_name_length(char *var)
{
    uint32_t length = 0;
    
    while(true)
    {
        if(*(var++) == '=') break;
        length++;
    }

    return length;
}



namespace environment
{
    char *update_env_block(char *old_env_block,
                           char **env_new_info, uint32_t env_new_info_count,
                           AllocatorType allocator_type = PERSISTENT)
    {
        // Compute new env block length
        uint32_t   old_env_block_length = get_env_block_length(old_env_block);
        uint32_t        new_info_length = get_env_new_info_length(env_new_info, env_new_info_count);
        uint32_t   new_env_block_length = old_env_block_length + new_info_length;

        // Allocate and prepare pointers
        char *    new_env_block = memory::allocate<char>(new_env_block_length, PERSISTENT);
        char *new_env_block_ptr = new_env_block;
        char *old_env_block_ptr = old_env_block;
        
        while(true)
        {
            // Get current variable and variable name lengths
            uint32_t var_name_length = get_var_name_length(old_env_block_ptr);
            uint32_t  old_var_length = string::length(old_env_block_ptr);
            uint32_t  new_var_length = old_var_length;

            // Copy over old content
            memcpy(new_env_block_ptr, old_env_block_ptr, old_var_length);

            // Add ntc to the end of the var
            new_env_block_ptr[new_var_length] = 0;

            // Move pointers and check for end
            new_env_block_ptr += new_var_length + 1;
            old_env_block_ptr += old_var_length + 1; // Skip NTC
            if (*old_env_block_ptr == 0) break;
        }

        // Add new info
        for (uint32_t i = 0; i < env_new_info_count; ++i)
        {
            uint32_t new_info_length = string::length(env_new_info[i]);
            memcpy(new_env_block_ptr, env_new_info[i], new_info_length);
            new_env_block_ptr[new_info_length] = 0;

            new_env_block_ptr += new_info_length + 1;
        }

        // Add final ntc
        new_env_block[new_env_block_length - 1] = 0;

        return new_env_block;
    }

    char *get_current_env_block()
    {
        return GetEnvironmentStrings();
    }

    void free_env_block(char *env_block)
    {
        FreeEnvironmentStrings(env_block);
    }

    void print_env_block(char *env_block)
    {
        while(true)
        {
            printf("%s\n", env_block);
            env_block += string::length(env_block) + 1;
            if(*env_block == 0) break;
        }
    }
}