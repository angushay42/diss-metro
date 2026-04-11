#ifndef UART_H
#define UART_H

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "ringbuffer.h"
#include "common-defines.h"
#include "dependencies.h"

// USART2 is the only one that can use ST-Link
#define UART            (USART2)
#define UART_PORT       (GPIOA)
#define UART_TX_PIN     (GPIO2)
#define UART_RX_PIN     (GPIO3)

#define UART_AF_MODE    (GPIO_AF7)
#define UART_BAUD_RATE  (115200) // from Google


extern int uart_setup(void);

extern int uart_write_many(uint16_t *data);
extern int uart_write_once(uint16_t data);
extern int uart_read(uint16_t *byte);


#endif  // UART_H