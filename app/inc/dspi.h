#ifndef DSPI_H
#define DSPI_H


#include "common-defines.h"

#include <libopencm3/stm32/spi.h>
#include "dependencies.h"

#define DSPI            (SPI1)
#define DSPI_PORT       (GPIOB)
#define DSPI_SCK        (GPIO3)
#define DSPI_MISO       (GPIO4)
#define DSPI_MOSI       (GPIO5)
// manual toggle
#define DSPI_CS_PORT    (GPIOB)
#define DSPI_CS_PIN     (GPIO10)


extern error_t dspi_setup(void);
extern error_t dspi_teardown(void);
extern void dspi_rcv(short *data);


#endif // DSPI_H