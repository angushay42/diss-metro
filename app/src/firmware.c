// own defines
#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"

#define ERROR_LED_PORT     (GPIOC) 
#define ERROR_LED_PIN      (GPIO12)


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void delay_cycles(uint32_t delay_cycles) {
    for (uint32_t i = 0; i < delay_cycles; i++)
        __asm volatile ("NOP");
}

static void delay_ms(double ms) {
    size_t i, n;

    // 13 too slow?
    for (i = 0, n = (size_t) roundf(ms * CPU_FREQ / 1000 / 13); i < n; i++)
        __asm volatile ("NOP");
}

// todo
static error_t error_handle(error_t err) {
    // disable interrupts
    duart_teardown();
    dspi_teardown();
    dmetro_teardown();

    

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

    uint16_t tempo;

    while (1) {
        delay_cycles(84000000 / 4);
        if ((err = dmetro_get_tempo_reading(&tempo)))
            return error_handle(err);
        
        if (dmetro_get_tempo() != tempo)
            if ((err = dmetro_set_tempo(tempo))) 
                return error_handle(err);
        duart_write_once(tempo);
        error_handle(MAIN_LOOP);
    }
    return 0;
}
