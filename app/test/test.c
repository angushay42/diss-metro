#include <stddef.h>
#include <stdio.h>
#include <math.h>

int main(void) {
    double input, hz;
    int bpm;
    __uint32_t psc;

    input = 1281.423;
    bpm = 145;
    hz = bpm / 60.0;
    psc = (__uint32_t) roundf((float) (80000000 / 65535) / hz);
    
    printf("%u\n", psc);
    psc = roundf((float) (80000000 / 65535) / hz);
    
    printf("%u\n", psc);
    
    return 0;
}