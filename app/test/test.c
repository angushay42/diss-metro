#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "ringbuffer.h"



int convert(uint16_t input) {
    uint16_t mask, mask2, mask3;
    int converted;
    mask = 1 << 12;
    mask2 = (uint16_t) ~( 15 << 12);
    mask3 = (uint16_t) ~(7 << 12);
    // keep 4msb as 0
    if (!(mask & input))
        return (int) (mask2 & input); 
    return (int) (mask3 & input);
}

short convert2(uint16_t input) {
    short output;
    if (!((1 << 12) & input)) {
        printf("positive\n");
        return output = input;
    }
    printf("negative\n");
    output = (((uint16_t) (15 << 12)) | input);
    return output;
}

//todo expand this more
void test_split(uint16_t data) {
    printf("first half (LSB) %u\n", (uint8_t) data);
    printf("first half (LSB) %u\n", (uint8_t) (data >> 8));
}

int test_ring_buffer() {
    ring_buf_t rb;
    uint16_t word, data, buffer[RING_BUF_MAX];
    int err;

    if ((err = ring_buf_setup(&rb, buffer, RING_BUF_MAX))) {
        return 1;
    }

    // write a word to buffer
    if ((err = ring_buf_write(&rb, (word = 1097))))
        return 2;

    // read the only word present
    if ((err = ring_buf_read(&rb, &data))|| data != 1097)
        return 3;

    // buffer should be empty
    if (!ring_buf_empty(&rb)) 
        return 4;

    // write a word and check if empty
    if ((err = ring_buf_write(&rb, word)) && !ring_buf_empty(&rb))
        return 5;
    
    
    printf("%u\n", data);
    

    return 0;
}

int main(void) {
    uint16_t data;
    int err;

    data = 1097;
    test_split(data);

    if ((err = test_ring_buffer())) {
        printf("RINGBUFFER FAIL with code: %i\n", err);
        return 1;
    }
    
    
    return 0;
}