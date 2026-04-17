#include "fft.h"

/* reverse the bits in unsigned integer x */
uint16_t bit_reverse(uint16_t x, uint32_t lg2n) {
    // reverse the bits in a word
    uint16_t r;
    r = 0;
    for (uint16_t i = 0; i < lg2n; i++) {
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

/** 
 * implemented from Wikipedia. Takes *SIGNED* short as input, 
 * computes as complex numbers.
 * computs the Radix-2 Decimation in Time
 * 
 * @param a - input array directly from ADC (signed short)
 * @param A - output array preallocated (signed complex)
 * @param N - size of DFT to perform.
 */ 
extern void fft(double a[], complex_t A[], uint32_t N) {
    uint32_t s, k, j, lg2n;
    double m;
    complex_t w_m, w, t, u;

    lg2n = int_log2(N);


    // copy input and reverse, also convert to complex
    for (k = 0; k < N; k++)
        A[(size_t) bit_reverse((uint16_t) k, lg2n)] = (complex_t) a[k];
    

    // from s=1 to log2(n)
    for (s = 1; s <= lg2n; s++) {
        m = pow((double) 2, (double) s);
        w_m = cexp((2.0 * PI * I) / m);
        for (k = 0; k < N; k += m) {
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