#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
// #include <libopencm3/stm32/i2c.h>

#define LED_PORT (GPIOA)  // port is a collection of pins (16 in this case)
#define LED_PIN (GPIO5)   // pin is the connection to the LED


#define CS_PIN (GPIO6)
#define CS_PORT (GPIOB)
#define TIM_ADC (RCC_TIM1)  // TODO

/*
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

// static void orig_gpio_setup(void) {
//     rcc_periph_clock_enable(RCC_GPIOA);
//     gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED_PIN);
// }

// static void delay_cycles(uint32_t cycles) {
//     for (uint32_t i = 0; i < cycles; i++) {
//       __asm__ ("nop");
//     }
// }

/* Might not even need this as CLK is for SPI... */
// static void tim_setup(void) { 
//     // clock division is given by:
//     // CLK / 1.7 (both in MHz)
//     // rounded down ofc, which we can give ahead of time as
//     // 49
//     uint32_t clk_div = 49;
//     timer_set_mode(TIM_ADC, clk_div, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);

// }

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

// configure a pin to read analog input for the potentiometer, see
// what it outputs?


// adc must be brought high before going low, this is selecting
// 

int main(void) {
    rcc_setup();
    gpio_setup();
    spi_setup();

    // orig_gpio_setup();

    int i = 0;
    while (1) {
        spi_rcv();
        // gpio_clear(CS_PORT, CS_PIN);    // pull low to initiate transfer  
        // spi_write(SPI1, 0xFF);
        // gpio_set(CS_PORT, CS_PIN);
        // gpio_toggle(LED_PORT, LED_PIN);
        // delay_cycles(84000000/ 4);
    }
    return 0;
}