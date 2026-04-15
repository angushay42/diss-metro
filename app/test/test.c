#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ringbuffer.h"
#include "fft.h"

/*************** prototypes ****************/
int test_ring_buffer();
int test_fft();
int test_split();
void print_complex(char *s, complex_t x);
/*************** endprototypes ****************/


void print_complex(char *s, complex_t z) {
    printf("%s = %.1f%+.1fi\n", s, creal(z), cimag(z));
}

int test_fft() {
    uint32_t size;
    uint16_t test, test_arr[8];
    complex_t res1, res2;

    short test2;
    size_t i, err;

    test = (uint16_t) -432;
    test2 = -432;
    size = 8;

    
    if ( (res1 = (complex_t) test) == (res2 = (complex_t) test2)) {
        return 1;
    }

    uint16_t res_arr[] = {0, 4, 2, 6, 1, 5, 3, 7};

    for (i = 0; i < size; i++) {
        test_arr[i] = bit_reverse((uint16_t) i, int_log2(size));
    }
    
    err = 0;
    for (i = 0; i < size; i++) {
        err |= !(test_arr[i] == res_arr[i]);
    }
    if (err)
        return 2;
    
    // perform 16-point dft 
    size = 16;
    double test_sin[size];
    

    // sine wave sampled at 44100Hz
    printf("generating sine wave\n");
    // fill sine wave
    for (i = 0; i < size; i++) {
        test_sin[i] = sin(2.0 * PI * 440.0 * (double) i / 44100.0);
    }
    printf("sine wave generated\n");
    
    
    complex_t stack_res[size];
    printf("testing stack fft\n");
    // test stack
    fft(test_sin, stack_res, size);
    printf("stack fft tested\n");

    for (i = 0; i<size; i++) {
        printf("frequency domain at [%zu]: %f\n", i, creal(stack_res[i]));
    }
    if (stack_res[0] != 1)  // unity amplitude (1)
        return 3;
    
    complex_t *heap_res = malloc((size_t) (sizeof(complex_t) * size));
    if (heap_res == NULL)
        return 4;

    printf("testing heap fft\n");
    // test heap
    fft(test_sin, heap_res, size);
    printf("heap fft tested\n");
    if (heap_res[0] != 1) {  // unity amplitude (1)
        free(heap_res);
        return 3;
    }

    free(heap_res);

    
    return (err = 0);
}

short convert(uint16_t input) {
    short output;
    if (!((1 << 12) & input)) {
        printf("positive\n");
        return output = input;
    }
    printf("negative\n");
    output = (((uint16_t) (15 << 12)) | input);
    return output;
}

// todo expand this more
int test_split() {
    int err;
    uint16_t data;

    data = 1097;
    printf("first half (LSB) %u\n", (uint8_t) data);
    printf("first half (LSB) %u\n", (uint8_t) (data >> 8));
    return (err = 0);
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
    int err;

    if ((err = test_ring_buffer())) {
        printf("RINGBUFFER FAIL with code: %i\n", err);
        return 1;
    }

    if ((err = test_fft())) {
        printf("FFT FAIL with code %i\n", err);
        return 2;
    }
    
    
    return 0;
}