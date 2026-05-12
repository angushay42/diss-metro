#ifndef TEST_H
#define TEST_H

#include "common-defines.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"

// source 
#include "dringbuffer.h"
#include "dfft.h"

#define MAX_BPM     (220)
#define MIN_BPM     (40)

// prototypes
extern int test_error_handle();
extern int test_dmetro();
extern int test_duart();
extern int test_sys_time();
extern int test_fft();
extern int test_delay();
int test_stack(void);

// helpers
void print_complex(char *s, complex_t x);
void print_line(size_t len);
void confirm_called(char *s);
void print_bits(uint8_t b);

#endif // TEST_H