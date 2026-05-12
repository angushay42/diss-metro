#include "test.h"


/* I remember monotonic stack from a LeetCode question, I think CarFleet? */
struct mono_stack {
    size_t size;
    size_t idx;
    uint16_t *data;
};

error_t stack_pop(struct mono_stack *p, uint16_t *data) {
    if ((*p).idx == 0)
        return MONO_STACK_EMPTY;
    
    *data = (*p).data[--((*p).idx)];
    return OK;
}

error_t stack_push(struct mono_stack *p, uint16_t data) {
    if ((*p).idx + 1 == (*p).size)
        return MONO_STACK_FULL;

    (*p).data[((*p).idx)++] = data;
    return OK;
}

error_t stack_peek(struct mono_stack *p, uint16_t *data) {
    if ((*p).idx == 0)
        return MONO_STACK_EMPTY;
    *data = (*p).data[(*p).idx-1];
    return OK;
}


int test_stack(void) {
    error_t err;
    size_t i, n;
    i = 0, n = 10;
    uint16_t expected, res, arr[n];

    struct mono_stack stack = {
        .data = arr,
        .size = n,
        .idx = 0
    };

    // test stack empty
    if ((err = stack_peek(&stack, &res))!= MONO_STACK_EMPTY)
        return 1;
    
    if ((err = stack_pop(&stack, &res))!= MONO_STACK_EMPTY)
        return 2;
    
    expected = 4390;
    // test stack push
    if ((err = stack_push(&stack, expected)))
        return 3;
    if (stack.data[0] != expected)
        return 4;
    
    // test peek
    if ((err = stack_peek(&stack, &res)))
        return 5;
    if (res != expected) {
        printf("res: %u\n", res);
        return 6;
    }
    
    // now test pop
    if ((err = stack_pop(&stack, &res)))
        return 7;
    if (res != expected)
        return 8;

    // fill stack
    for (i = 0; i < n; i++)
        stack_push(&stack, i * 3);

    // test stack full
    if ((err = stack_push(&stack, 21))!= MONO_STACK_FULL)
        return 9;
    return 0;
} 

// int test_mono_stack(void) {
//     /* only difference here is stack must be in one direction */
//     // 8, 5, 4, 1 = len of 4
//     uint16_t example[] = {8, 1, 2, 5, 3, 4, 1};
//     uint16_t expected = 4;
//     struct mono_stack stack

// }