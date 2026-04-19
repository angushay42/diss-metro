#ifndef DRING_BUF_H
#define DRING_BUF_H

#include "common-defines.h"

//todo cross platform for testing?

#define RING_BUF_MAX (128)      // benchmark this

/*********** ringbuffer  *************/
typedef struct ring_buf_t {
    uint16_t* buffer;    // pointer to buffer
    uint32_t head;      // head index (write)
    uint32_t tail;      // tail index (read)
    uint32_t mask;      // HAS to be (2^n) - 1
} ring_buf_t;

extern error_t dring_buf_setup(ring_buf_t* rb, uint16_t* buffer, uint32_t size);
extern int dring_buf_empty(ring_buf_t* rb);
extern error_t dring_buf_write(ring_buf_t* rb, uint16_t byte);
extern error_t dring_buf_read(ring_buf_t* rb, uint16_t* byte);

#endif // DRING_BUF_H