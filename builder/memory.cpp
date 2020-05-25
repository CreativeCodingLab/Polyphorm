struct Allocator
{
    void *data;
    uint32_t size;
    uint32_t top;
};

typedef uint32_t AllocatorState;

enum AllocatorType
{
    TEMPORARY,
    PERSISTENT
};

namespace memory
{
    static Allocator allocator_temporary;
    static Allocator allocator_persistent;

    static const uint32_t init_memory_size = 1000000;
    static void init()
    {
        allocator_temporary.data = malloc(init_memory_size);
        allocator_temporary.size = init_memory_size;
        allocator_temporary.top = 0;

        allocator_persistent.data = malloc(init_memory_size);
        allocator_persistent.size = init_memory_size;
        allocator_persistent.top = 0;
    }
    
    template <typename T>
    static T *allocate(uint32_t count, AllocatorType type = TEMPORARY)
    {
        Allocator *allocator = type == TEMPORARY ? &allocator_temporary : &allocator_persistent;
        
        if(allocator->top + count > allocator->size)
        {
            allocator->size *= 2;
            allocator->data = realloc(allocator->data, allocator->size);
        }

        T *result = (T *)((char *)allocator->data + allocator->top);
        allocator->top += sizeof(T) * count;

        return result;
    }

    static AllocatorState set(AllocatorType type = TEMPORARY)
    {
        Allocator *allocator = type == TEMPORARY ? &allocator_temporary : &allocator_persistent;
        return allocator->top;
    }

    static void reset(AllocatorState state, AllocatorType type = TEMPORARY)
    {
        Allocator *allocator = type == TEMPORARY ? &allocator_temporary : &allocator_persistent;
        allocator->top = state;
    }
};
