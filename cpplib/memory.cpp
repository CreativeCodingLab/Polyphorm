#include "memory.h"
#include <cstring>

struct Stack {
    StackAllocatorState *data;
    uint32_t top;
    uint32_t size;
};

// `stack` namespace handles operations on Stack.
namespace stack {
    // Initialize stack and allocate initial memory necessary for stack of size `size`
    void init(Stack *stack, uint32_t size) {
        stack->size = size;
        stack->top = 0;
        stack->data = memory::alloc_heap<StackAllocatorState>(size);
    }

    // Return an initialized stack of size `size`
    Stack get(uint32_t size) {
        Stack stack;
        stack::init(&stack, size);
        if(!stack.data) {
            stack.size = 0;
        }
        return stack;
    }

    // Reset the stack
    // Does not free any memory
    void reset(Stack *stack) {
        stack->top = 0;
    }

    // Push a new item from stack
    void push(Stack *stack, StackAllocatorState item) {
        // In case stack is full, reallocate
        if(stack->size == stack->top) {
            StackAllocatorState *new_mem = memory::alloc_heap<StackAllocatorState>(stack->size * 2);
            // Maybe not the best way to handle the problem?
            if(!new_mem) {
                return;
            }
            memcpy(new_mem, stack->data, sizeof(StackAllocatorState) * stack->size);
            memory::free_heap(stack->data);
            stack->data = new_mem;
            stack->size *= 2;
        }

        stack->data[stack->top++] = item;
    }

    // Pop an item from stack
    StackAllocatorState pop(Stack *stack) {
        // Cannot pop if no items in stack
        if(stack->top == 0){
            return StackAllocatorState();
        }
        return stack->data[--stack->top];
    }
}

StackAllocator allocator_temp = memory::get_stack_allocator(MEGABYTES(10));
Stack temp_state_stack = stack::get(10);

StackAllocator memory::get_stack_allocator(uint32_t size) {
    StackAllocator allocator = {};
    allocator.storage = malloc(size);
    if(!allocator.storage) {
        return allocator;
    }
    allocator.size = size;
    return allocator;
}

StackAllocatorState memory::save_stack_state(StackAllocator *allocator) {
    return allocator->top;
}

void memory::load_stack_state(StackAllocator *allocator, StackAllocatorState state) {
    allocator->top = state;
}

StackAllocator *memory::get_temp_stack() {
    return &allocator_temp;
}

void memory::push_temp_state() {
    StackAllocatorState temp_state = memory::save_stack_state(get_temp_stack());
    stack::push(&temp_state_stack, temp_state);
}

void memory::pop_temp_state() {
    StackAllocatorState temp_state = stack::pop(&temp_state_stack);
    memory::load_stack_state(memory::get_temp_stack(), temp_state);
}

void memory::free_temp() {
    memory::get_temp_stack()->top = 0;
}

void memory::free_heap(void *ptr) {
    free(ptr);
}
