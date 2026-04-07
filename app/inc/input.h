#ifndef INPUT_H
#define INPUT_H

#include <libopencm3/stm32/spi.h>

#include "common-defines.h"

#define CS_PORT (GPIOA)
#define CS_PIN (GPIO4)


extern void dspi_setup(void);
extern void dspi_rcv();
extern int dspi_read_once(uint16_t *data);



#endif // INPUT_H   