// libopencm3 defines
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>        // from LowByteProductions
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

// FreeRTOS defines
// #include "FreeRTOS.h"

#define CS_PIN (GPIO6)
#define CS_PORT (GPIOB)
#define TIM_ADC (RCC_TIM1)  // TODO

/* UART defines */
// stm32f401 uses USART2 (from nucleo user manual)
#define UART_PORT (GPIOA)
#define UART_TX_PIN (GPIO2)
#define UART_RX_PIN (GPIO3)
#define UART_BAUD_RATE (115200) // from Google

/* game plan
get input from pot (tempo)
    maybe default value?
generate pulse at provided tempo
get input from guitar
calculate difference between inputs
report ?
*/

static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

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

/* adapted from https://github.com/lowbyteproductions/bare-metal-series */
static void uart_setup(void) {
    // enable clock too
    rcc_periph_clock_enable(RCC_USART2);

    usart_set_mode(USART2, USART_MODE_TX_RX);   // duplex
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(USART2, UART_BAUD_RATE);
    usart_set_databits(USART2, 8);  // byte
    usart_set_stopbits(USART2, 1);
    usart_set_parity(USART2, USART_PARITY_NONE);

    usart_enable_rx_interrupt(USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);       // got interrupt, where handler?
    // found handler: https://libopencm3.org/docs/latest/stm32f4/html/group__CM3__nvic__isrprototypes__STM32F4.html
    // unsure if will be recieving much but good to have it here in case.
    

    // enable last, configure first?
    usart_enable(USART2);
}

/* might not need to do this ?*/
// static void systick_setup(void) {
//     // systick_set_frequency(84000000, );
    
// }

int main(void) {
    rcc_setup();
    gpio_setup();
    spi_setup();

    // orig_gpio_setup();

    int i = 0;
    while (1) {
        spi_rcv();
        
    }
    return 0;
}
