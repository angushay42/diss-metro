#include "test.h"


/* sys global variables */
volatile uint64_t sys_time;
uint32_t _resolution, _sys_psc;
/* tick time in ms*/
double scale_value;


static void delay_cycles(size_t cycles) {
    confirm_called("delay_cycles");
    printf("cycles: %zu\n", cycles);

    for (size_t i = 0; i < cycles; i++)
        __asm volatile ("NOP");
}

static void delay_ms(double ms) {
    confirm_called("delay_ms");
    long i, n;

    // Mac has ~2.7GHz frequency
    // so 2_700_000_000 / 1000 = 2_700_000 or 2700000.0
    // 13 clock cycles (roughly the cycles used for the function in test/scrap)
    for (i = 0, n = (long) round(ms * 2700000.0 / 13.0); i < n; i++) {
        __asm volatile ("NOP");
    }
}


void sys_tick_handler(void) {
    uint64_t temp64 = sys_time;
    sys_time = temp64 + _resolution;
}

/* resolution in ms */
error_t sys_setup(uint32_t resolution) {
    // systick should have 10.5MHz (84/8) 
    double input, output;
    uint32_t psc;

    // todo some check for res
    _resolution = resolution;
    input = 10500000.0;
    output = 1 / ( (double) resolution / 1000.0);
    psc = (uint32_t) roundf(input / output);
    if (psc > ((1 << 25)- 1))
        return SYSTICK_INVALID_RESOLUTION;
    _sys_psc = psc;
    scale_value = 1000.0 / input;   // in ms

    sys_time = 0;
    return OK;
}

static uint32_t systick_get_value(void) {
    return (uint32_t) 3000;
}

/* get time in ms*/
static uint64_t get_time() {
    uint64_t temp64 = 0;
    // (reload - curr) = ticks counted
    // 1 tick = 1/10.5mhz = 0.000000095238095 seconds
    temp64 = (_sys_psc - systick_get_value()) * scale_value;
    return sys_time + temp64;
}


extern int test_sys_time() {
    error_t err;
    uint64_t start, end, temp;


    if ((err = sys_setup(1000)))
        return 1;

    if ((err = sys_setup(10)))
        return 2;

    start = get_time();
    sys_tick_handler();
    temp = get_time();
    if (temp - start != 10)
        return 3;
    

    // printf("%u, %u, %f\n", _sys_psc, _resolution, scale_value);
    printf("get_time: %llu\n", get_time());
    return 0;
}


extern int test_delay() {
    int err;
    double ms;
    struct timespec start, end;
    time_t thresh, diff;

    clock_gettime(CLOCK_MONOTONIC, &start);
    delay_ms((double) (ms = 1000));
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    diff = (time_t) round(end.tv_nsec-start.tv_nsec);
    thresh = 500; // nanosec
    if ((time_t) fabs(diff - (ms * 1000.0)) <= thresh)
    if (round(diff) != (ms / 1000.0)){
        printf("difference: %ld\n", diff);
        return 1;
    }
    return 0;
}