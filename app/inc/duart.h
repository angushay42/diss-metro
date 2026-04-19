#ifndef DUART_H
#define DUART_H

#include "dringbuffer.h"
#include "common-defines.h"

#include "dependencies.h"
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>


// USART2 is the only one that can use ST-Link
#define UART            (USART2)
#define UART_PORT       (GPIOA)
#define UART_TX_PIN     (GPIO2)
#define UART_RX_PIN     (GPIO3)

#define UART_AF_MODE    (GPIO_AF7)
#define UART_BAUD_RATE  (115200) // from Google


extern error_t duart_setup(void);
extern error_t duart_teardown(void);

extern error_t duart_write_many(uint16_t *data);
extern error_t duart_write_once(uint16_t data);
extern error_t duart_read(uint16_t *byte);


#endif  // DUART_H