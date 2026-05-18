
#include "duart.h"
#include "dringbuffer.h"

static uint16_t _buffer[RING_BUF_MAX];
static ring_buf_t _rb;
uint8_t count = 0;

/* static prototypes */
static void duart_enable(void);
static void duart_disable(void);
static error_t duart_send(struct packet *p);
static error_t duart_send_8(struct packet *p);
static error_t duart_send_16(struct packet *p);
static error_t duart_send_32(struct packet *p);
static error_t duart_send_64(struct packet *p);

/** @TODO
 * it could be beneficial to "flatten" data I want to send and then just cycle it out.
 * i.e. each packet gets flattened to an identical format of bytes to be sent over uart?
 */

static error_t duart_send_8(struct packet *p) {
    uint8_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        usart_send_blocking(UART, (uint8_t) temp);
    }
    return OK;
}

static error_t duart_send_16(struct packet *p) {
    uint16_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

static error_t duart_send_32(struct packet *p) {
    uint32_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

static error_t duart_send_64(struct packet *p) {
    uint64_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

extern error_t duart_send_packet(struct packet *p) {
    error_t err;
    size_t n;

    if ((*p).id == NULL)
        return DUART_SEND_NULL;
    
    /** create packet for string identifier and send it */
    /* get length of id */
    for (n = 0; (*p).id[n] != '\0'; n++)
        ;
    // todo
    if (n > (size_t) 255)
        return DUART_INVALID_SIZE;

    
    struct packet ch = {
        .len = n,
        .size = 1,
        .is_signed = false,
        .u = (*p).id,
    };

    // send id
    if ((err = duart_send(&ch)))
        return err;

    // send data
    if ((err = duart_send(p)))
        return err;
    return OK;
}

// todo wish this was more general...
static error_t duart_send(struct packet *p) {
    if (p == NULL)
        return DUART_SEND_NULL;

    uint8_t flag;

    // send start char
    usart_send_blocking(UART, (uint8_t) '{');

    flag = (1 << ((*p).size / 2));
    if ((*p).is_signed)
        flag |= (1 << fsigned);

    // send metadata in flag
    usart_send_blocking(UART, (uint8_t) flag);

    // send length of data to come
    usart_send_blocking(UART, (uint16_t) (*p).len);

    switch ((*p).size) {
        case 1:
            duart_send_8(p);
            break;
        case 2:
            duart_send_16(p);
            break;
        case 4:
            duart_send_32(p);
            break;
        case 8:
            duart_send_64(p);
            break;
        default:
            return DUART_INVALID_SIZE;
    }
    
    // send stop char
    usart_send_blocking(UART, (uint8_t) '}');

    return OK;
}

error_t duart_read(uint16_t *word) {
    error_t err = dring_buf_read(&_rb, word);
    return err;
}

/************************* interrupts *********************/
void usart2_isr(void) {
    gpio_set(ERROR_LED_PORT, ERROR_LED_PIN);
    delay_ms(100);
    gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
    if (usart_get_flag(USART2, USART_FLAG_RXNE)) {
        gpio_set(ERROR_LED_PORT, ERROR_LED_PIN);
        delay_ms(100);
        gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
        uint16_t data = USART2_DR;
        // dring_buf_write(&_rb, usart_recv(USART2));
    }

}


/***************************** util ***********************/
static void duart_enable(void) {
    // USART2_CR1 |= USART_CR1_TE; 
}

static void duart_disable(void) {
    // USART2_CR1 &= ~USART_CR1_TE; // disable?
}

/********************* init functions ********************/
/* adapted from https://github.com/lowbyteproductions/bare-metal-series */
error_t duart_setup(void) {
    /* enable clock to the peripheral */
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);

    /* configure gpio pins*/
    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN | UART_RX_PIN);

    /* disble first */
    usart_disable(UART);

    /* config */
    usart_set_mode(UART, USART_MODE_TX_RX);   // duplex
    usart_set_flow_control(UART, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(UART, UART_BAUD_RATE);
    usart_set_databits(UART, 8);  // byte
    usart_set_stopbits(UART, 1);
    usart_set_parity(UART, USART_PARITY_NONE);

    /* enable rx interrupt on the USART and in NVIC */
    usart_enable_rx_interrupt(UART);
    nvic_enable_irq(NVIC_USART2_IRQ);

    /* clear flag in case */
    uint16_t data = USART2_DR;

    /* finally enable*/
    usart_enable(UART);

    // set up buffer
    error_t err = dring_buf_setup(&_rb, _buffer, MIN_PACKET_BUFFER_SIZE);
    return err;
}

extern error_t duart_teardown(void) {
    nvic_disable_irq(NVIC_USART1_IRQ);
    usart_disable_rx_interrupt(UART);
    rcc_periph_clock_disable(RCC_USART2);
    return OK;
}
/******************* endblock **********************/