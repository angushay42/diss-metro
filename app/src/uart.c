
#include "uart.h"
#include "ringbuffer.h"

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
    ring_buf_write(&_rb, usart_recv_blocking(UART));
}

/* sends an array over UAR with blocking */
int uart_write_many(uint16_t *data) {
    // copy ptr
    uint16_t *ptr = data;
    while (*ptr) {
        usart_send_blocking(UART, (uint8_t) *ptr);
        usart_send_blocking(UART, (uint8_t) (*ptr >> 8));
        ptr++;
    }
    return 0;   // OK
}

/* sends 1 byte over UART with blocking */
extern int uart_write_once(uint16_t data) {
    // split 16bit to two 8 bits
    usart_send_blocking(UART, (uint8_t) data);
    usart_send_blocking(UART, (uint8_t) (data >> 8));
    return 0;
}


/* blocking writes a byte into byte and returns 0 if successful */
int uart_read(uint16_t *word) {
    // todo what to do with error? 
    int err = ring_buf_read(&_rb, word);
    return err;
}


/* adapted from https://github.com/lowbyteproductions/bare-metal-series */
void uart_setup(void) {
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
    int err = ring_buf_setup(&_rb, _buffer, RING_BUF_MAX);
}
