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

void delay_cycles(uint32_t delay_cycles) {
    for (uint32_t i = 0; i < delay_cycles; i++)
        __asm volatile ("NOP");
}


int main(void) {
    rcc_setup();
    // dspi_setup();
    uart_setup();
    // metro_setup();

    uint16_t data;
    // char data;
    // char test[30] = "hello, world!\n";
    data = 0;
    while (1) {
        // dspi_rcv();
        // dspi_read_once(&data);
        // uart_write_once(data);
        // data++;
        uart_write_once(data+65);
        data = (data + 1) % 27;
        delay_cycles(84000000 / 4);
        // uart_write_many(test);
        
    }
    return 0;
}
