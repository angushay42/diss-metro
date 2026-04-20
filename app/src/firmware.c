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
 

void minimal_adc_setup() {
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // analog or AF?
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);

    adc_power_off(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_clk_prescale((uint32_t) 1);

    adc_set_single_conversion_mode(ADC1);
    adc_power_on(ADC1);     
    // manually toggle it down        
    adc_clear_flag(ADC1, ADC_SR_EOC);
}

error_t minimal_adc_read(uint32_t *data, uint32_t cycle_timeout) {
    uint32_t i;
    adc_set_regular_sequence(ADC1, 1, (uint8_t[]) {0});
    adc_start_conversion_regular(ADC1);
    for (i = 0; !(adc_eoc(ADC1)); i++)
        if (i > cycle_timeout) {
            *data = 1 << 14;
            return DADC_TIMEOUT;
        }

    *data = adc_read_regular(ADC1);
    return OK;
}


int main(void) {
    rcc_setup();
    error_t err;

    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, ERROR_LED_PIN);
    gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);

    minimal_adc_setup();

    
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
    
    uint32_t data, timeout;
    timeout = 400000;
    uint16_t tempo;

    while (1) {
        if ((err = minimal_adc_read(&data, timeout)))
            return error_handle(err);
        delay_ms(1000);
        duart_write_once(data);
        // if ((err = dmetro_get_tempo_reading(&tempo)))
        //     return error_handle(err);
        
        // if (dmetro_get_tempo() != tempo)
        //     if ((err = dmetro_set_tempo(tempo))) 
        //         return error_handle(err);
        // duart_write_once(tempo);
    }
    return 0;
}
