
#include "duart.h"
#include "dringbuffer.h"

static uint16_t _buffer[RING_BUF_MAX];
static ring_buf_t _rb;
uint8_t count = 0;


// UART sends LSB first

// ///todo what do i need to do here????
// // todo receiving 16bit words
// void usart2_isr(void) {

//     // gpio_toggle(ERROR_LED_PORT, ERROR_LED_PIN);
//     // todo receive interrupt
//     if (usart_get_flag(UART, USART_SR_TXE)) {
//         // flag is cleared by writing to the data register
//         USART2_DR = 1;
//     }
//     else if (usart_get_flag, USART_CR1_RXNEIE) {

//     }

// }


static void duart_enable(void) {
    // USART2_CR1 |= USART_CR1_TE; 
}

static void duart_disable(void) {
    // USART2_CR1 &= ~USART_CR1_TE; // disable?
}

extern error_t duart_write_bytes(char *data) {
    duart_enable();
    // copy ptr
    char *ptr = data;
    while (*ptr) {
        usart_send_blocking(UART, (uint8_t) *ptr);
        usart_send_blocking(UART, (uint8_t) (*ptr >> 8));
        ptr++;
    }
    duart_disable();
    return OK;

}
extern error_t duart_write_byte(char data) {
    duart_enable();
    usart_send_blocking(UART, (uint8_t) data);
    duart_disable();

    return OK;
}


/* sends an array over UAR with blocking */
extern error_t duart_write_many(uint16_t *data) {
    duart_enable();

    // copy ptr
    uint16_t *ptr = data;
    while (*ptr) {
        usart_send_blocking(UART, (uint8_t) *ptr++);
    }
    duart_disable();

    return OK;
}

extern error_t duart_start_sequence(uint8_t flag) {
    // uint8_t temp1, temp2;
    // temp1 = 1 << (fsigned -1);
    // temp1 = ~temp1;
    // temp2 = temp1 & flag;
    // uint8_t size = ~(1 << (fsigned)-1) & flag;
    uint8_t size = flag;
    if (size == 0 || size == 3 || (size > 4 && size < 8) || size > 8)
        return DUART_START_SEQUENCE;
    duart_enable();
    for (size_t i = 0; i < 3; i++)
        duart_write_byte(flag);
    duart_disable();
    
    return OK;
}   

/* sends 1 byte over UART with blocking */
extern error_t duart_write_once(uint16_t data) {
    duart_enable();

    // split 16bit to two bytes
    usart_send_blocking(UART, (uint8_t) data);
    usart_send_blocking(UART, (uint8_t) (data >> 8));
    duart_disable();

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

    usart_disable(UART);

    usart_set_mode(UART, USART_MODE_TX_RX);   // duplex
    usart_set_flow_control(UART, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(UART, UART_BAUD_RATE);
    usart_set_databits(UART, 8);  // byte
    usart_set_stopbits(UART, 1);
    usart_set_parity(UART, USART_PARITY_NONE);

    // usart_enable_rx_interrupt(UART);
    // usart_enable_tx_interrupt(UART);
    // nvic_enable_irq(NVIC_USART2_IRQ);
    // usart_disable_tx_interrupt(UART);
    // usart_disable_tx_complete_interrupt(UART);
    
    // print out the control register interrupts to see if any are set?
    // PEIE
    // TXEIE
    // TCIE
    // RXNEIE
    // IDLEIE
    // CTSIE
    // EIE
    // if (USART2_CR1 & USART_CR1_PEIE)
    //     error_handle(1);
    // if (USART2_CR1 & USART_CR1_TXEIE)
    //     error_handle(2);
    // if (USART2_CR1 & USART_CR1_TCIE)
    //     error_handle(3);
    // if (USART2_CR1 & USART_CR1_RXNEIE)
    //     error_handle(4);
    // if (USART2_CR1 & USART_CR1_IDLEIE)
    //     error_handle(5);
    // if (USART2_CR3 & USART_CR3_CTSIE)
    //     error_handle(6);
    // if (USART2_CR3 & USART_CR3_EIE)
    //     error_handle(7);


    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN | UART_RX_PIN);
    
    usart_enable(UART);
    duart_enable();

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
