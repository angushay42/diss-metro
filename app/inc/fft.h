#ifndef FFT_H
#define FFT_H

#include <math.h>
#include <complex.h>
#include "common-defines.h"

// convenience
#define PI (3.14159265358979323846)
typedef double complex complex_t;


uint32_t int_log2(uint32_t x);
uint16_t bit_reverse(uint16_t x, uint32_t lg2n);
extern void fft(double a[], complex_t A[], uint32_t N);

 

#endif  // FFT_H