#include "dsystime.h"


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

void sys_teardown(void) {
    systick_interrupt_disable();
    systick_counter_disable();
}

/* get time in ms*/
extern uint64_t get_time(bool precise) {
    uint64_t temp64 = 0;
    // (reload - curr) = ticks counted
    // 1 tick = 1/10.5mhz = 0.000000095238095 seconds
    if (precise) 
        temp64 = (uint64_t) round((double) (_psc - systick_get_value()) * scale_value);
    return sys_time + temp64;
}