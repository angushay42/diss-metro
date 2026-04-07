// libopencm3 defines
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

// own defines
#include "diss-uart.h"
#include "metronome.h"
#include "common-defines.h"

#define CS_PORT (GPIOA)
#define CS_PIN (GPIO4)

static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

//todo move this to modular files
static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    // /********** spi setup ************/
    // // SCLK is A5
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO5);
    // // MISO is A6
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO6);
    // // MOSI is A7
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO7);
    // // CS is A4 
    // gpio_mode_setup(CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, CS_PIN);

    // // these are given by datasheet, thankfully all in same AF column
    // gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
    // /******** endSPI  *****************/


    /********** UART setup ************/
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN); 
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_RX_PIN); 

    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    /******** endUART  *****************/
}

static void spi_setup(void) {
    spi_init_master(
        SPI1, 
        SPI_CR1_BAUDRATE_FPCLK_DIV_64, 
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1,
        SPI_CR1_DFF_16BIT,
        SPI_CR1_MSBFIRST
    );
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1); // tell stm it is always master
    spi_enable(SPI1);
}

static void spi_rcv(void) {
    // write blank statement to ADC to start transfer (Claude)
    gpio_clear(CS_PORT, CS_PIN);
    spi_xfer(SPI1, 0x0001);
    // set gpio pin high
    gpio_set(CS_PORT, CS_PIN);
}

void delay_cycles(uint32_t cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++) {
        __asm__ volatile ("NOP");
    }
}


int main(void) {
    rcc_setup();
    gpio_setup();
    spi_setup();
    uart_setup();
    metro_setup();

    while (1) {
        // todo
        xTaskCreate(0,0,0,0,0,0);
        
        // uart_write();    
    }
    return 0;
}
