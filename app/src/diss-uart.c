
#include "diss-uart.h"


static uint8_t buffer[RING_BUF_MAX];

static ring_buf_t _rb = {
    .buffer = buffer,
    .head = 0,
    .tail = 0,
    .mask = RING_BUF_MAX - 1    // this seems correct
};


void usart1_isr(void) {
    // when we receive something we want to read it to a data byte
    // // p515 RM, can be cleared by software read or set to 0 by software
    // bool flag = usart_get_flag(USART2, USART_SR_RXNE);      // todo, keep flag?

    // software read should clear RXNE bit

    // from: https://www.embedded.com/5-best-practices-for-writing-interrupt-service-routines/
    // use __inline__ to avoid function calls.

    // todo I think this should be revisted when FreeRTOS is setup, 
    // that way I can pass this off to a non-isr call AND handle the error.
    ring_buf_write(&_rb, (uint8_t) usart_recv_blocking(USART2));
}

/* sends an array over UAR with blocking */
int uart_write(uint8_t *data) {
    // copy ptr
    uint8_t *ptr = data;
    while (*ptr) {
        usart_send_blocking(USART2, (uint16_t) *ptr++);    // have to cast?
    }
    return 0;   // OK
}

/* blocking writes a byte into byte and returns 0 if successful */
int uart_read(uint8_t *byte) {
    // todo what to do with error? 
    int err = ring_buf_read(&_rb, byte);
    return err;
}

/* adapted from https://github.com/lowbyteproductions/bare-metal-series */
void uart_setup(void) {
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

/* initialise ring buffer. returns 0 if successful */
int ring_buf_setup(ring_buf_t* rb, uint8_t* buffer, uint32_t size) {
    if (rb == NULL || buffer == NULL || (size & (size+1)) != 0) // todo check
        return 1;
    
    rb->buffer = buffer;
    rb->mask = size - 1;
    rb->head  = 0;      // could do this in one line, unsure...
    rb->tail = 0;
    return 0;
}

/* returns 0 if false */
int ring_buf_empty(ring_buf_t* rb) {
    return rb->head == rb->tail;    // 0 is false
}

/* write a byte into ring buffer. returns 0 if successful */
int ring_buf_write(ring_buf_t* rb, uint8_t byte) {
    if (((rb->head + 1) & rb->mask) == rb->tail)  // buffer full
        return 1;
    
    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) & rb->mask;
    return 0;   // success
}

/* read a byte from ring buffer. returns 0 if successful */
int ring_buf_read(ring_buf_t* rb, uint8_t* byte) {
    if (rb->tail == rb->head)
        return 1;       // err code
    
    // deref and copy byte
    *byte = rb->buffer[rb->tail];

    // increment pointer
    rb->tail = (rb->tail + 1) & rb->mask;   // only works if power of 2.
    return 0;
}