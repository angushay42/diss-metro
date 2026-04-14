#ifndef ANALYSE_H
#define ANALYSE_H

#include <math.h>
#include <complex.h>
#include "common-defines.h"

// convenience
#define PI (3.14159265358979323846)
typedef double complex complex_t;


uint32_t int_log2(uint32_t x);
uint16_t bit_reverse(uint16_t x, uint32_t lg2n);
void fft(short *a, uint32_t N);

 

#endif  // ANALYSE_H