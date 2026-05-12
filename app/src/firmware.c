#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"
#include "dsystime.h"

static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

extern void delay_ms(double ms) {
    size_t i, n;

    // 7 gives 99 ms 
    for (i = 0, n = (size_t) round(ms * CPU_FREQ / 1000 / 7); i < n; i++)
        __asm volatile ("NOP");
}


extern error_t error_handle(error_t err) {
    // disable interrupts
    duart_teardown();
    dspi_teardown();
    dmetro_teardown();
    sys_teardown();

    
    // loop forever and toggle the ERROR LED with the code
    while (1) {
        for (size_t i = 0; i < (size_t) err; i++) {
            gpio_set(ERROR_LED_PORT, ERROR_LED_PIN);
            delay_ms(750);
            gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
            delay_ms(750);
        }
        delay_ms(5000); // 5 seconds
    }
    // should never reach
    return err;
}

// global variable for firmware.c
// declared in common-defines.h ONCE
// *defined* here ONCE 
volatile bool test_started = false;

extern void tempo_smoothing_test(void) {
    error_t err;
    uint32_t delay, start, stop;
    float rate = 3.0;   // bpm / second 

    start = MIN_BPM, stop = MAX_BPM;
    delay = (uint32_t) (1.0 / rate * 1000.0);    // period = 1/freq * 1000 = ms time period
    
    // increase tempo at rate r
    while (start <= stop) {
        if ((err = dmetro_set_tempo((uint16_t) start)))
            error_handle(err);
        delay_ms(delay);
        start++;
    }
    
    // decrease tempo at rate r
    start = MAX_BPM, stop=MIN_BPM;
    while (start >= stop) {
        if ((err = dmetro_set_tempo((uint16_t) start)))
            error_handle(err);
        delay_ms(delay);
        start--;
    }
}

int main(void) {
    rcc_setup();
    error_t err;

    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, ERROR_LED_PIN);
    gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
    
    if ((err = dspi_setup())) {
        error_handle(err);
        return err;
    }
    if ((err = duart_setup())) {
        error_handle(err);
        return err;
    }
    if ((err = dmetro_setup())) {
        error_handle(err);
        return err;
    }

    if ((err = sys_setup(10)))
        return error_handle(err);
    uint64_t last_poll, poll_period, now;
    bool started = false;
    uint16_t reading;
    uint8_t max_size = 64;
    uint16_t tempo_copy;
    uint64_t stamps[max_size];
    short samples[max_size];

    struct packet samples_pack = {
        .id = "SAMPLE",
        .is_signed = true,
        .size = sizeof(short),
        .len = 0,
        .u = samples,
    }, 
    stamps_pack = {
        .id = "STAMP",
        .is_signed = false,
        .size = sizeof(uint64_t),
        .len = 0,
        .u = stamps
    }, 
    tempo_pack = {
        .id = "TEMPO",
        .u = &tempo_copy,
        .size = sizeof(tempo_copy),
        .len = 1,
        .is_signed = false,
    },
    reading_pack = {
        .id = "READING",
        .u = &reading,
        .size = sizeof(reading),
        .len = 1,
        .is_signed = false
    };

    nvic_set_priority(NVIC_TIM4_IRQ, 1);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 2);
    
    poll_period = 100;  // responsive enough?

    size_t sample_idx, stamp_idx;
    stamp_idx = sample_idx = 0;

    /* just send one for now */
    samples_pack.len = 1;
    stamps_pack.len = 1;

    
    // temporary superloop for testing smoothing
    // init testing var
    test_started = false;
    dmetro_stop();
    while (1) {
        if (test_started == true) {
            tempo_smoothing_test();
        }
    }

    // while (1) {
        
    //     // dspi_rcv(samples);
    //     // duart_send_packet(&samples_pack);
    //     // *stamps = get_time(false);
    //     now = get_time(false);
    //     if (!started) {
    //         last_poll = now;
    //         started = true;
    //     }
    //     // duart_send_packet(&stamps_pack);
    //     // if now - previous >= poll_period
    //     if (now - last_poll >= poll_period) {
    //         last_poll = now;
    //         if ((err = dmetro_get_tempo_reading(&reading, 40))) 
    //             return error_handle(err);
    //         if (reading != (tempo_copy = dmetro_get_tempo())) {
    //             dmetro_set_tempo(reading);
    //             duart_send_packet(&tempo_pack);
    //             duart_send_packet(&reading_pack);
    //         }
    //     }
    // }
    return 0;
}
