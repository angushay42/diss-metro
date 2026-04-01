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
    uart_read();
}

int uart_send_blocking(ring_buf_t *rb) {
    // gah whats the flow?
    // the actual send is 1 byte. 
    // but we have a buffer to read from first
    // buffer is written to,
    uint8_t data; 
    while (ring_buf_read(rb, &data) == 0) {
        usart_send_blocking(USART2, (uint16_t) data);    // have to cast?
    }
}

int uart_read() {
    ring_buf_read(&_rb, usart_recv(USART2));
}

void test_uart(void) {
    // wait for input from user (button press)
    // on button press, write to uart
    char test_message[30] = "Hello, world!";     // remember weird string literal rules
    
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