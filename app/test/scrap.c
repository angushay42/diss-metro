#include <math.h>

static void delay_ms(double ms) {
    size_t i, n;

    for (i = 0, n = (size_t) round(ms * 84000.0); i < n; i++)
        __asm volatile ("NOP");
}

int main(void) {
    return 0;
}