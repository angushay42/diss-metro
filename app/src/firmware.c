// libopencm3 defines
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>        // from LowByteProductions
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

#include "diss-uart.h"

// FreeRTOS defines
// #include "FreeRTOS.h"

#define CS_PIN (GPIO6)
#define CS_PORT (GPIOB)
#define TIM_ADC (RCC_TIM1)  // TODO


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /********** spi setup ************/
    // SCLK is A5
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO5);
    // MISO is A6
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO6);
    // MOSI is A7
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO7);
    // CS is B6
    gpio_mode_setup(CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, CS_PIN);

    // these are given by datasheet, thankfully all in same AF column
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);

    //todo needed?
    /********** UART setup ************/
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN); 
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_RX_PIN); 

    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);

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

/* might not need to do this ?*/
static void systick_setup(void) {
    // systick_set_frequency(84000000, );
    
}

int main(void) {
    rcc_setup();
    gpio_setup();
    spi_setup();
    uart_setup();


    uint8_t test_message[30] = "Hello, world!";     // remember weird string literal rules
    uint8_t data;
    int err;

    while (1) {
        // spi_rcv();
        // uart_read(data);
        uart_write(test_message);
        
        // if ((err = uart_read(&data)) == 0)
        //     break;
    }
    return err;
}
