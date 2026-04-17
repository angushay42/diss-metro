// libopencm3 defines
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>


// own defines
#include "common-defines.h"
#include "dependencies.h"
#include "uart.h"
#include "metronome.h"
#include "input.h"
#include "fft.h"


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void delay_cycles(uint32_t delay_cycles) {
    for (uint32_t i = 0; i < delay_cycles; i++)
        __asm volatile ("NOP");
}


int main(void) {
    rcc_setup();

    if (dspi_setup())
        return 1;
    if (uart_setup())
        return 2;
    if (metro_setup())
        return 3;


    uint32_t size = 256;
    uint16_t buffer[size], *bufp;

    uint32_t i, j, k;

    // todo double is 8 bytes so how to send that over?
    for (; ;) {
        // bufp = buffer;
        // for (j = 0; j < size; j++) {
        // // fill buffer
        //     dspi_rcv(bufp++);  
        // }
    }
    return 0;
}
