#ifndef INC_DISS_UART_H
#define INC_DISS_UART_H

#include "common-defines.h"
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>

#define UART_PORT (GPIOA)
#define UART_TX_PIN (GPIO2)
#define UART_RX_PIN (GPIO3)
#define UART_AF_MODE (GPIO_AF7)
#define UART_BAUD_RATE (115200) // from Google


#define RING_BUF_MAX (128)      // benchmark this

/*********** ringbuffer  *************/
typedef struct ring_buf_t {
    uint8_t* buffer;    // pointer to buffer
    uint32_t head;      // head index (write)
    uint32_t tail;      // tail index (read)
    uint32_t mask;      // HAS to be (2^n) - 1
} ring_buf_t;

int ring_buf_setup(ring_buf_t* rb, uint8_t* buffer, uint32_t size);
int ring_buf_empty(ring_buf_t* rb);
int ring_buf_write(ring_buf_t* rb, uint8_t byte);
int ring_buf_read(ring_buf_t* rb, uint8_t* byte);


/********** UART ***********/
extern void uart_setup(void);

extern int uart_write(uint8_t *data);
extern int uart_read(uint8_t *byte);

#endif  // INC_DISS_UART_H