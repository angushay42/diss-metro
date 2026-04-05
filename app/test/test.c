#include <stddef.h>
#include <stdio.h>

int main(void) {
    double input, hz;
    int bpm;

    input = 1281.423;
    bpm = 120;

    for (int i = 0; i < 20; i++) {
        hz = (double) (bpm - i) / 60.0;
        printf("%i bpm %f hz\n", bpm-i, hz);
        printf("psc: %u\n", (__uint32_t) (input / hz));
    }
    return 0;
}