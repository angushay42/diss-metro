#include "fft.h"

/* reverse the bits in unsigned integer x */
uint8_t bit_reverse(uint16_t x) {
// reverse the bits in a word
    uint8_t r;
    r = 0;
    while (x) {
        // extract bit from right to left and build r from left to right
        r = (r << 1) | (x & 1);
        x >>= 1;
    }
    return r;
}

/* simple unsigned int log2 function */
uint32_t int_log2(uint32_t x) {
    uint32_t log = 0;
    // divide by 2 until less than 0
    while (x >>= 1) {log++;}
    return log;
}


/* implemented from Wikipedia. Takes *SIGNED* short as input, computes as complex numbers. */
/* computs the Radix-2 Decimation in Time*/
void fft(short *a, uint32_t N) {
    uint32_t s, k, j;
    double m;
    complex_t w_m, w, t, u;

    complex_t A[N];
    // copy input and reverse, also convert to complex
    for (k = 0; k < N; k++) {
        A[bit_reverse( (uint16_t) k)] = (complex_t) a[k];
    }

    // from s=1 to log2(n)
    for (s = 1; s < log2(N); s++) {
        m = pow((double) 2, (double) s);
        w_m = exp((2.0 * PI * I) / m);
        for (k = 0; k < N; k++) {
            w = 1;
            for (j = 0; j < m/2; j++) {
                t = w * A[(int) (k + j + m/2)];
                u = A[(int) (k + j)];
                A[(int) (k + j)] = u + t;
                A[(int) (k + j + m/2)] = u - t;
                w = w * w_m;
            }
        }
    }

}