#ifndef INPUT_H
#define INPUT_H

#include <libopencm3/stm32/spi.h>

#include "common-defines.h"
#include "dependencies.h"

#define DSPI            (SPI1)
#define DSPI_PORT       (GPIOB)
#define DSPI_SCK        (GPIO3)
#define DSPI_MISO       (GPIO4)
#define DSPI_MOSI       (GPIO5)
// manual toggle
#define DSPI_CS_PORT    (GPIOB)
#define DSPI_CS_PIN     (GPIO6)


extern void dspi_setup(void);
extern void dspi_rcv(void);
extern int dspi_read_once(uint16_t *data);



#endif // INPUT_H   