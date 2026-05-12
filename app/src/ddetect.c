#include "ddetect.h"

/* adjust how sensitive detection is. */
uint32_t threshold;
uint32_t average;
/** LSB = (2 * V_Ref) / 8192
 * from DS21700E-page 12 
 */
double LSB;




error_t ddettect_setup(void) {
    threshold = (uint32_t) 350;
    average = 0;
    LSB = 2.0 * 2.5 / 8192.0;

    return OK;
}


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
    *data = (*p).data[(*p).idx];
    return OK;
}

/** TODO: Unsure if this will work... */
/* Detect a note from a given set of samples. Returns 0 if successful. n must be greater than 1. */
extern bool ddetect_detect_note(uint64_t *note_stamp, bool *ans, short *samples, size_t n) {
    /**
     * Plan:
     * take n samples of guitar input
     * get a probability for if a note was played (like 0.8 is 80% certain its a note)
     * if probability is above some threshold *AND* the timestamp of this note probability
     * is within _another_ threshold, then we will say the user played a note correctly. 
     */
    if (note_stamp == NULL || ans == NULL || samples == NULL)
        return DDETECT_NULL_INPUT;
    if (n <= 1)
        return DDETECT_INVALID_SIZE;

    
    // 
    uint16_t temp, amp, arr[n];
    error_t err;
    struct mono_stack stack = {
        .size = n,
        .data = arr,
        .idx = 0,
    };
    // push first sample onto stack
    if ((err = stack_push(&stack, (uint16_t) abs(samples[0]))))
        return err;
    
    for (size_t i = 1; i < n; i++) {
        amp = (uint16_t) abs(samples[i]);

        // keep popping from stack if current value is lesser
        while (!stack_peek(&stack, &temp) && amp < temp) {
            stack_pop(&stack, &temp);
        }
        err = stack_push(&stack, amp);


    }
    return OK;
}
