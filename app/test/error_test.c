#include "test.h"

// error handle global variables
static int test_led_on, test_led_off;


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



static error_t error_handle(error_t err, time_t timeout, long count) {
    time_t now, start = time(NULL);
    long i = 0;
    
    test_led_on = test_led_off = 0;

    // loop forever and toggle the ERROR LED with the code
    while (1) {
        now = time(NULL);
        if (timeout > (time_t) 0 && (now - start) >= timeout) {
            err = ERROR_HANDLE_TIMEOUT;
            break;
        }
        if (i >= count) {
            err = ERROR_HANDLE_TIMEOUT;
            break;
        }
        for (size_t j = 0; j < (size_t) err; j++) {
            //todo swap
            test_led_on++;
            printf("LED ON\n");
            delay_ms(500);
            test_led_off++;
            printf("LED OFF\n");
        }
        delay_ms(2500);
        i++;
    }
    // should never reach
    return err;
}


extern int test_error_handle() {
    int err;
    long count;
    error_t input_err, return_err;
    time_t timeout, thresh, actual, expected;
    struct timespec start, end;

    /* test timeout*/
    timeout = 1, count = -1;
    if ((return_err = error_handle(1, timeout, count)) != ERROR_HANDLE_TIMEOUT)
        return (err = 1); 

    /* test count param */
    timeout = -1, count = 1;
    if ((return_err = error_handle(1, timeout, count)) != ERROR_HANDLE_TIMEOUT)
        return (err = 2); 

    /* test with actual errors */
    input_err = MAIN_LOOP, timeout = -1, count = 1;
    if ((return_err = error_handle(input_err, timeout, count)) != ERROR_HANDLE_TIMEOUT 
        && (test_led_off != (int) input_err || test_led_on != (int) input_err)
    )
        return (err = 3); 

    input_err = DFFT_INVALID_ARGS, timeout = -1, count = 1;
    if ((return_err = error_handle(input_err, timeout, count)) != ERROR_HANDLE_TIMEOUT 
        && (test_led_off != (int) input_err || test_led_on != (int) input_err)
    )
        return (err = 4); 

    /* test total time taken */
    input_err = DFFT_INVALID_N, timeout = -1, count = 1;
    thresh = 5000;  // ns
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    return_err = error_handle(input_err, timeout, count);
    clock_gettime(CLOCK_MONOTONIC, &end);

    actual = end.tv_nsec - start.tv_nsec;
    // delays 2.5 seconds per count
    // delays 0.5 seconds per blink
    expected = ((time_t)( 2.5 * 1000000)* count) + ((time_t)( 0.5 * 1000000 * input_err));
    
    if ((actual - expected) > thresh) {
        printf("diff in nanoseconds: %ld\n", end.tv_nsec - start.tv_nsec);
        return (err = 4); 
    }


    return (err = 0);
}

