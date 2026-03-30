#ifndef INC_DISS_UART_H
#define INC_DISS_UART_H

#include "common-defines.h"

typedef struct ring_buf_t {
    uint8_t* buffer;    // pointer to buffer
    uint32_t head;      // head index (write)
    uint32_t tail;      // tail index (read)
    uint32_t mask;      // HAS to be power of 2
} ring_buf_t;

int ring_buf_setup(ring_buf_t* rb, uint8_t* buffer, uint32_t size);
int ring_buf_empty(ring_buf_t* rb);
int ring_buf_write(ring_buf_t* rb, uint8_t byte);
int ring_buf_read(ring_buf_t* rb, uint8_t* byte);

#endif  // INC_DISS_UART_H