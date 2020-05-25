#include "memory.h"
#include "stack.h"

static StackAllocator allocator_temp = memory::get_stack_allocator(MEGABYTES(10));
static Stack<StackAllocatorState> temp_state_stack = stack::get<StackAllocatorState>(10);

StackAllocator memory::get_stack_allocator(uint32_t size)
{
    StackAllocator allocator = {};
    allocator.storage = malloc(size);
    assert(allocator.storage);
    allocator.size = size;
    return allocator;
}

StackAllocatorState memory::save_stack_state(StackAllocator *allocator)
{
    return allocator->top;
}

void memory::load_stack_state(StackAllocator *allocator, StackAllocatorState state)
{
    allocator->top = state;
}

StackAllocator *memory::get_temp_stack()
{
    return &allocator_temp;
}

void memory::push_temp_state()
{
    StackAllocatorState temp_state = memory::save_stack_state(get_temp_stack());
    stack::push(&temp_state_stack, temp_state);
}

void memory::pop_temp_state()
{
    StackAllocatorState temp_state = stack::pop(&temp_state_stack);
    memory::load_stack_state(memory::get_temp_stack(), temp_state);
}

void memory::free_temp()
{
    memory::get_temp_stack()->top = 0;
}

void memory::free_heap(void *ptr)
{
    free(ptr);
}
