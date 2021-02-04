#pragma once
#include <stdint.h>
#include <cstdlib>

#define KILOBYTES(kB) (kB * 1024)
#define MEGABYTES(mB) (mB * 1024 * 1024)

struct StackAllocator {
    void *storage;
    uint32_t size;
    uint32_t top = 0;
};

typedef uint32_t StackAllocatorState;

extern StackAllocator allocator_temp;

namespace memory {
    template <typename T>
    T *alloc_heap(uint32_t count) {
        T *mem = (T *)malloc(count * sizeof(T));
        return mem;
    }

    void free_heap(void *ptr);

    StackAllocator get_stack_allocator(uint32_t size);
    StackAllocatorState save_stack_state(StackAllocator *allocator);
    void load_stack_state(StackAllocator *allocator, StackAllocatorState state);

    template <typename T>
    T *alloc_stack(StackAllocator *allocator, uint32_t count) {
        T *result = (T *)((char *)allocator->storage + allocator->top);
        if(allocator->top + count > allocator->size) return NULL; 
        allocator->top += count * sizeof(T);
        return result;
    }

    StackAllocator *get_temp_stack();

    void push_temp_state();
    void pop_temp_state();
    
    template <typename T>
    T *alloc_temp(uint32_t count) {
        return memory::alloc_stack<T>(memory::get_temp_stack(), count);
    }

    void free_temp();
}

#ifdef CPPLIB_MEMORY_IMPL
#include "memory.cpp"
#endif