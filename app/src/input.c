
// todo get dsp library or function?
#include "input.h"
#include "ringbuffer.h"


static uint16_t _buffer[RING_BUF_MAX];
static ring_buf_t _rb;

extern void dspi_setup(void) {
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
        SPI_CR1_BAUDRATE_FPCLK_DIV_64, 
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

    // set ring buffer up
    ring_buf_setup(&_rb, _buffer, RING_BUF_MAX);
}

extern void dspi_rcv(void) {
    // write blank statement to ADC to start transfer (Claude)
    gpio_clear(DSPI_CS_PORT, DSPI_CS_PIN);
    ring_buf_write(&_rb, spi_xfer(DSPI, (uint16_t) 0x1));
    // set gpio pin high
    gpio_set(DSPI_CS_PORT, DSPI_CS_PIN);

}

extern int dspi_read_once(uint16_t *data) {
    if (ring_buf_read(&_rb, data))
        return 1;
    return 0;
}


void spi1_isr(void) {
    if (SPI1_SR & SPI_SR_RXNE) {
        int err;
        gpio_set(GPIOA, GPIO8);
        uint16_t temp = spi_read(DSPI);
        // first 2 bits are dont care
        // 3rd bit is null
        uint16_t mask = (uint16_t) 15 << 12;
        // if positive, set 4MSB to 0
        if (!((1 << 12) & temp))
            temp &= ~mask;
        else 
            temp |= mask;
        // todo handle error
        err = ring_buf_write(&_rb, temp);
    }
}