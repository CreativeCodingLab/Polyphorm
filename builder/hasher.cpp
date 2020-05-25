const uint32_t HASH_DB_CHUNK_SIZE = 50;
struct HashDBChunk
{
    char *strings[HASH_DB_CHUNK_SIZE];
    uint32_t hashes[HASH_DB_CHUNK_SIZE];

    uint32_t size     = 0;
    uint32_t capacity = HASH_DB_CHUNK_SIZE;
    HashDBChunk *next_chunk = NULL;
};

struct HashDB
{
    HashDBChunk data;
};

char *find_in_table(HashDB *table, uint32_t hash)
{
    HashDBChunk *current_chunk = &(table->data);
    do
    {
        for(uint32_t i = 0; i < current_chunk->size; ++i)
        {
            if (hash == current_chunk->hashes[i])
            {
                return current_chunk->strings[i];
            }
        }
        current_chunk = current_chunk->next_chunk;
    } while(current_chunk);
    return NULL;
}

bool check_collision(HashDB *table, uint32_t hash, char *string, uint32_t string_length, char **collision_string)
{
    char *string_with_same_hash = find_in_table(table, hash);
    if (string_with_same_hash && !string::equals(string_with_same_hash, string, string_length))
    {
        *collision_string = string_with_same_hash;
        return true;
    }
    return false;
}

void add_to_table(HashDB *table, uint32_t hash, char *string, uint32_t string_length)
{
    HashDBChunk *current_chunk = &(table->data);
    do
    {
        if (current_chunk->size < current_chunk->capacity)
        {
            uint32_t current_size = current_chunk->size;
            current_chunk->hashes[current_size]  = hash;
            current_chunk->strings[current_size] = string::create_null_terminated(string, string_length, PERSISTENT);

            current_chunk->size++;
            if (current_chunk->size == current_chunk->capacity)
            {
                current_chunk->next_chunk = memory::allocate<HashDBChunk>(1, PERSISTENT);
            }
            break;
        }
        current_chunk = current_chunk->next_chunk;
    } while(current_chunk);
}

uint32_t hash_string(char *string, uint32_t string_length)
{
    uint32_t hash_value = 5381;
    for(uint32_t i = 0; i < string_length; ++i)
    {
        hash_value = ((hash_value << 5) + hash_value) + string[i];
    }

    return hash_value + 1;
}

void hash_file(char *file_path, HashDB *hash_db, char *backup_dir = NULL)
{
    auto mem_state_temp  = memory::set(TEMPORARY);
    File file = file_system::read_file(file_path, TEMPORARY);

    if(backup_dir)
    {
        auto mem_state = memory::set(TEMPORARY);
        char *file_name        = file_system::get_file_name_from_path(file_path);
        char *backup_file_path = file_system::get_combined_paths(backup_dir, file_name, TEMPORARY);
        file_system::copy_file(file_path, backup_file_path);
        memory::reset(mem_state, TEMPORARY);
    }

    // Allocate destination file
    uint32_t dst_file_size = file.size * 2;
    char *src_file       = (char *)file.data;
    char *src_file_end   = (char *)file.data + file.size;
    char *dst_file       = memory::allocate<char>(dst_file_size, TEMPORARY);
    char *dst_file_start = dst_file;
    while(true)    
    {
        if (dst_file_size < (uint32_t)(dst_file - dst_file_start))
        {
            // When not enough memory allocated, double the size of the buffer. This is done
            // by allocating block of memory of the same size as existing block, right after
            // the already allocated block. This works because there is nothing being
            // TEMPORARY-ily allocated in the loop.
            memory::allocate<char>(dst_file_size, TEMPORARY);
            dst_file_size *= 2;
        }
        uint32_t dst_file_size_left = dst_file_size - (uint32_t)(dst_file - dst_file_start);

        if(string::equals(src_file, "HASH(\"", 6))
        {
            src_file += 6;

            // Extract string to hash
            char *string_start     = src_file;
            uint32_t string_length = 0;
            while(*src_file++ != '"') //"
            {
                string_length++;
            }
            
            // Hash the string
            uint32_t hash = hash_string(string_start, string_length);

            // Check hash for collision
            char *collision_string = NULL;
            bool collided = check_collision(hash_db, hash, string_start, string_length, &collision_string);
            if (collided)
            {
                printf("Collision happened during hashing between strings \"%.*s\" and \"%s\"! Hashing of the file %s aborted!\n", string_length, string_start, collision_string, file_path);
                memory::reset(mem_state_temp, TEMPORARY);
                return;
            }

            // Store hash and string to hash db
            add_to_table(hash_db, hash, string_start, string_length);

            // Prepare the string to write results to
            auto pre_format_state         = memory::set(TEMPORARY);
            uint32_t format_string_length = 6 + string_length + 10 + 1;
            char *format_string_prefix = "HASH(\"";
            char *format_string_suffix = "\", %#010x)";
            char *format_string        = memory::allocate<char>(format_string_length, TEMPORARY);
            memcpy(format_string, format_string_prefix, 6);
            memcpy(format_string + 6, string_start, string_length);
            memcpy(format_string + 6 + string_length, format_string_suffix, 10);
            format_string[format_string_length - 1] = 0;

            // Write the string into a file
            sprintf_s(dst_file, dst_file_size_left, format_string, hash);
            memory::reset(pre_format_state, TEMPORARY);
            
            // Move beyond the string
            dst_file += format_string_length + 4 - 1;
            while(*src_file++ != ')');
        }
        else
        {
            *(dst_file++) = *(src_file++);
        }

        if(src_file == src_file_end)
        {
            break;
        }
    }

    file_system::write_file(file_path, dst_file_start, (uint32_t)(dst_file - dst_file_start));
    memory::reset(mem_state_temp, TEMPORARY);
}