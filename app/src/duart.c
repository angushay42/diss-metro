
#include "duart.h"
#include "dringbuffer.h"

static uint16_t _buffer[RING_BUF_MAX];
static ring_buf_t _rb;


// UART sends LSB first

// todo receiving 16bit words
void usart2_isr(void) {
    // when we receive something we want to read it to a data byte
    // // p515 RM, can be cleared by software read or set to 0 by software
    // bool flag = usart_get_flag(USART2, USART_SR_RXNE);      // todo, keep flag?

    // software read should clear RXNE bit

    // from: https://www.embedded.com/5-best-practices-for-writing-interrupt-service-routines/
    // use __inline__ to avoid function calls.

    // todo I think this should be revisted when FreeRTOS is setup, 
    // that way I can pass this off to a non-isr call AND handle the error.
    dring_buf_write(&_rb, usart_recv_blocking(UART));
}


extern error_t duart_write_bytes(char *data) {
    // copy ptr
    char *ptr = data;
    while (*ptr) {
        usart_send_blocking(UART, (uint8_t) *ptr);
        usart_send_blocking(UART, (uint8_t) (*ptr >> 8));
        ptr++;
    }
    return OK;

}
extern error_t duart_write_byte(char data) {
    usart_send_blocking(UART, (uint8_t) data);
    return OK;
}


/* sends an array over UAR with blocking */
extern error_t duart_write_many(uint16_t *data) {
    // copy ptr
    uint16_t *ptr = data;
    while (*ptr) {
        usart_send_blocking(UART, (uint8_t) *ptr++);
    }
    return OK;
}

/* sends 1 byte over UART with blocking */
extern error_t duart_write_once(uint16_t data) {
    // split 16bit to two bytes
    usart_send_blocking(UART, (uint8_t) data);
    usart_send_blocking(UART, (uint8_t) (data >> 8));
    return OK;
}

/* blocking writes a byte into byte and returns 0 if successful */
error_t duart_read(uint16_t *word) {
    error_t err = dring_buf_read(&_rb, word);
    return err;
}


/* adapted from https://github.com/lowbyteproductions/bare-metal-series */
error_t duart_setup(void) {
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);
    
    /********** GPIO ************/
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN | UART_RX_PIN); 

    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    /******** endGPIO  *****************/

    usart_set_mode(UART, USART_MODE_TX_RX);   // duplex
    usart_set_flow_control(UART, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(UART, UART_BAUD_RATE);
    usart_set_databits(UART, 8);  // byte
    usart_set_stopbits(UART, 1);
    usart_set_parity(UART, USART_PARITY_NONE);

    usart_enable_rx_interrupt(UART);
    nvic_enable_irq(NVIC_USART1_IRQ);
    // found handler: https://libopencm3.org/docs/latest/stm32f4/html/group__CM3__nvic__isrprototypes__STM32F4.html
    // unsure if will be recieving much but good to have it here in case.
    
    usart_enable(UART);

    // set up buffer
    error_t err = dring_buf_setup(&_rb, _buffer, RING_BUF_MAX);
    return err;
}

extern error_t duart_teardown(void) {
    nvic_disable_irq(NVIC_USART1_IRQ);
    usart_disable_rx_interrupt(UART);
    rcc_periph_clock_disable(RCC_USART2);
    return OK;
}
