// libopencm3 defines
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

// own defines
#include "common-defines.h"
#include "uart.h"
#include "metronome.h"
#include "input.h"


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}



void delay_cycles(uint32_t cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++) {
        __asm__ volatile ("NOP");
    }
}

int main(void) {
    rcc_setup();
    spi_setup();
    uart_setup();
    metro_setup();

    uint16_t data;

    while (1) {
        spi_rcv();
        dspi_read_once(&data);
        uart_write_once(data);
    }
    return 0;
}
