#include "ringbuffer.h"

/* initialise ring buffer. returns 0 if successful */
extern int ring_buf_setup(ring_buf_t* rb, uint16_t* buffer, uint32_t size) {
    if (rb == NULL || buffer == NULL || (size & (size+1)) != 0) // todo check
        return 1;
    
    rb->buffer = buffer;
    rb->mask = size - 1;
    rb->head  = 0;      // could do this in one line, unsure...
    rb->tail = 0;
    return 0;
}

/* returns 0 if false */
extern int ring_buf_empty(ring_buf_t* rb) {
    return rb->head == rb->tail;    // 0 is false
}

/* write a byte into ring buffer. returns 0 if successful */
extern int ring_buf_write(ring_buf_t* rb, uint16_t byte) {
    if (((rb->head + 1) & rb->mask) == rb->tail)  // buffer full
        return 1;
    
    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) & rb->mask;
    return 0;   // success
}

/* read a byte from ring buffer. returns 0 if successful */
extern int ring_buf_read(ring_buf_t* rb, uint16_t* byte) {
    if (rb->tail == rb->head)
        return 1;       // err code
    
    // deref and copy byte
    *byte = rb->buffer[rb->tail];

    // increment pointer
    rb->tail = (rb->tail + 1) & rb->mask;   // only works if power of 2.
    return 0;
}