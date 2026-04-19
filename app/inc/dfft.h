#ifndef FFT_H
#define FFT_H

#include <math.h>
#include <complex.h>
#include "common-defines.h"

// convenience
#define PI (3.14159265358979323846)
typedef double complex complex_t;


extern error_t fft(double a[], complex_t A[], uint32_t N);

 

#endif  // FFT_H