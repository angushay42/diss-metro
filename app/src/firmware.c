#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"
#include "dsystime.h"
#include "ddetect.h"

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


uint64_t detect_arr[2];
struct packet detected_packet = {
    .is_signed = false,
    .id = "DETECT",
    .len = 2,
    .size = sizeof(uint64_t),
    .u = detect_arr
};

static void report_results(uint64_t note_stamp, bool ans) {
    detect_arr[0] = note_stamp;
    detect_arr[1] = (uint64_t) ans;
    duart_send_packet(&detected_packet);
}

uint64_t stamp;
short sample;
struct packet stampp = {
    .id = "STAMP",
    .u = &stamp,
    .len = 1,
    .size = sizeof(stamp),
    .is_signed = false,
},
samplep = {
    .id = "SAMPLE",
    .u = &sample,
    .len = 1,
    .size = sizeof(sample),
    .is_signed = true
};

static void send_stamped_sample(void) {
    // get one sample
    dspi_rcv(&sample);
    
    // get a stamp
    stamp = get_time(false);

    // // send both
    // duart_send_packet(&stampp);
    // duart_send_packet(&samplep);
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
    
    nvic_set_priority(NVIC_TIM4_IRQ, 1);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 2);
    
    size_t sample_idx, sample_size, max_size;
    max_size = 64;
    uint64_t note_stamp, next_beat, now, listening_period;
    short samples[max_size], sample;
    bool ans;

    listening_period = 50; // ms
    sample_size = 10;       

    while (1) {
        // dspi_rcv(&sample);
        send_stamped_sample();

        // duart_send_packet(&samples_pack);
        // *stamps = get_time(false);
        // dmetro_poll_update((uint64_t) 250); 
    }
    return 0;
}
