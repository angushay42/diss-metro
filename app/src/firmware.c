// own defines
#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void delay_cycles(uint32_t delay_cycles) {
    for (uint32_t i = 0; i < delay_cycles; i++)
        __asm volatile ("NOP");
}

// todo
static void delay_ms(double ms) {
    double tick_to_ms = 1.0 / 84000.0;
    delay_cycles((ms * tick_to_ms) / 4);
}

// todo
static void error_handle(error_t err) {
    // disable interrupts 
    // loop forever and toggle the ERROR LED with the code
}


int main(void) {
    rcc_setup();
    error_t err;

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
        dmetro_get_tempo_reading(&tempo);
        if (dmetro_get_tempo() != tempo)
            dmetro_set_tempo(tempo);    // todo error
        duart_write_once(tempo);
    }
    return 0;
}
