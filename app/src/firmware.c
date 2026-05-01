#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"


#include <libopencm3/cm3/systick.h>





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

/* time in ms */
volatile uint64_t sys_time;
uint32_t _resolution, _psc;

/* tick time in ms*/
double scale_value;


static uint32_t tick_count = 0;

void sys_tick_handler(void) {
    sys_time += _resolution;

    if (++tick_count >= 100) {
        gpio_toggle(ERROR_LED_PORT, ERROR_LED_PIN);
        tick_count = 0;
    }

}

/**
 * 
 * @param resolution in ms
 */
error_t sys_setup(uint32_t resolution) {
    // systick should have 10.5MHz (84/8) 
    double input, output;
    uint32_t psc;

    sys_time = 0;
    if (sys_time)
        return SYSTICK_INIT_ERROR;
    // todo some check for res
    _resolution = resolution;
    input = 10500000.0;
    output = 1 / ( (double) resolution / 1000.0);
    scale_value = 1000.0 / input;   // in ms


    psc = (uint32_t) round(input / output);
    if (psc > ((1 << 25)- 1))
        return SYSTICK_INVALID_RESOLUTION;
    _psc = psc;
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    systick_set_reload(psc);
    systick_interrupt_enable();
    systick_counter_enable();
    return OK;
}

/* get time in ms*/
static uint64_t get_time(bool precise) {
    uint64_t temp64 = 0;
    // (reload - curr) = ticks counted
    // 1 tick = 1/10.5mhz = 0.000000095238095 seconds
    if (precise) 
        temp64 = (uint64_t) round((double) (_psc - systick_get_value()) * scale_value);
    return sys_time + temp64;
}

static void send_sample() {
    dspi_rcv(&data);
    data = get_time(false);
    // data = 1029;
    size = sizeof(data);
    duart_start_sequence(size);
    duart_write_byte((uint8_t) 1);
    for (i = 0; i < size; i++) {
        duart_write_once((uint16_t) data);
        data >>= 16;
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


    uint64_t stamp;
    uint16_t sample;
    size_t size, i;
    
    while (1) {
        
    }
    return 0;
}
