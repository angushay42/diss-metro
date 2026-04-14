#include "input.h"

extern int dspi_setup(void) {
    int err;

    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOB);

    /********** gpio *********** */
    gpio_mode_setup(DSPI_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, DSPI_SCK | DSPI_MISO);
    gpio_mode_setup(DSPI_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, DSPI_CS_PIN);

    // these are given by datasheet, thankfully all in same AF column
    gpio_set_af(DSPI_PORT, GPIO_AF5, DSPI_SCK | DSPI_MISO);
    /******** endGPIO  *****************/

    spi_init_master(
        DSPI, 
        SPI_CR1_BAUDRATE_FPCLK_DIV_64,   // changed from 64 for testing 
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1,
        SPI_CR1_DFF_16BIT,
        SPI_CR1_MSBFIRST
    );
    spi_enable_software_slave_management(DSPI);
    spi_set_nss_high(DSPI); // tell stm it is always master
    spi_enable(DSPI);

    // nvic_enable_irq(NVIC_SPI1_IRQ);
    // spi_enable_rx_buffer_not_empty_interrupt(DSPI);

    return (err = 0);
}

/* store signed 13 bit integer into data. */
extern void dspi_rcv(short *data) {
    uint16_t temp;
    
    // write blank statement to ADC to start transfer (Claude)
    gpio_clear(DSPI_CS_PORT, DSPI_CS_PIN);
    
    // get raw bits from adc
    temp = spi_xfer(DSPI, (uint16_t) 0x1);
    
    // convert to actual value (13 bit to short, with sign)
    *data = (short) convert_from_adc(temp);

    // set gpio pin high
    gpio_set(DSPI_CS_PORT, DSPI_CS_PIN);
}

/* convert 13bit adc integer to a usable word */
static uint16_t convert_from_adc(uint16_t input) {
    uint16_t mask = (uint16_t) 15 << 12;
    // if positive, set 4MSB to 0
    if (!((1 << 12) & input))
        return input & (~mask);
    else 
        return input | mask;
}


// todo still needed?
void spi1_isr(void) {
    if (SPI1_SR & SPI_SR_RXNE) {
        gpio_set(GPIOA, GPIO8);     // could work
        uint16_t temp = spi_read(DSPI);
        // first 2 bits are dont care
        // 3rd bit is null
        uint16_t mask = (uint16_t) 15 << 12;
        // if positive, set 4MSB to 0
        if (!((1 << 12) & temp))
            temp &= ~mask;
        else 
            temp |= mask;
        
    }
}