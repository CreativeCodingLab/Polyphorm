namespace string
{
    uint32_t length(char *string)
    {
        return (uint32_t) strlen(string);
    }

    char *create_null_terminated(char *string, uint32_t string_length, AllocatorType allocator_type = PERSISTENT)
    {
        char *nt_string = memory::allocate<char>(string_length + 1, allocator_type);
        memcpy(nt_string, string, string_length);
        nt_string[string_length] = 0;
        return nt_string;
    }

    bool equals(char *string1, char *string2, uint32_t max_length)
    {
        return strncmp(string1, string2, max_length) == 0;
    }

    bool equals(char *string1, char *string2)
    {
        return strcmp(string1, string2) == 0;
    }
}
