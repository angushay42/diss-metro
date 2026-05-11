#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"
#include "dsystime.h"


// todo float is 32 bits


static error_t send_sample(void);


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

extern void delay_ms(double ms) {
    size_t i, n;

    // 13 too slow?
    for (i = 0, n = (size_t) round(ms * CPU_FREQ / 1000 / 13); i < n; i++)
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


static error_t send_sample() {
    error_t err;
    uint64_t stamp;
    short sample;
    size_t size, i;
    // get sample
    dspi_rcv(&sample);
    // get time
    stamp = get_time(false);

    // // start sequence
    // size = sizeof(stamp); 
    // duart_start_sequence(size);
    // duart_write_byte((uint8_t) 1);
    // for (i = 0; i < size; i++) {
    //     duart_write_once((uint16_t) stamp);
    //     stamp >>= 16;
    // }
    sample = -320;
    delay_ms(1000);
    size = sizeof(sample);
    // size |= (1 << fsigned);   // samples are signed!
    if ((err = duart_start_sequence(size | (1 << fsigned))))
        return err;
    if ((err = duart_write_byte((uint8_t) 1)))
        return err;
    for (i = 0; i < size; i++) {
        if ((duart_write_once((uint16_t) sample)))
            return err;
        sample >>= 16;
    }
    return OK;
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
    
    uint8_t max_size = 64;
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
    };

    size_t sample_idx, stamp_idx;
    stamp_idx = sample_idx = 0;

    /* just send one for now */
    samples_pack.len = 1;
    stamps_pack.len = 1;

    uint16_t reading;
    while (1) {
        dspi_rcv(samples);
        duart_send_packet(&samples_pack);
        *stamps = get_time(false);
        duart_send_packet(&stamps_pack);
        if ((err = dmetro_get_tempo_reading(&reading, 0)))
            return error_handle(err);
        if (reading != dmetro_get_tempo()) {
            dmetro_set_tempo(reading);
            dmetro_report_tempo();
        }
    }
    return 0;
}
