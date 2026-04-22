#ifndef INC_COMMON_DEFINES_H
#define INC_COMMON_DEFINES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* LED2 on Nucleo board */
#define TEST_LED_PORT       (GPIOA)
/* LED2 on Nucleo board */
#define TEST_LED_PIN        (GPIO5)




/* Error enum. Each error will have its own value. */
typedef enum d_errors {
    OK,
    DSPI_SETUP,
    DADC_SETUP,
    DMETRO_SETUP,
    DUART_SETUP,
    MAIN_LOOP,
    DRINGBUF_SETUP,
    DRINGBUF_EMPTY,
    DRINGBUF_FULL,
    DRINGBUF_INVALID_RB,
    DRINGBUF_INVALID_BUFFER,
    DRINGBUF_INVALID_SIZE,
    DMETRO_INVALID_TEMPO,
    DMETRO_INVALID_PSC,
    DFFT_INVALID_ARGS,  // expand out
    DFFT_INVALID_N,
    DADC_TIMEOUT,
    ERROR_HANDLE_TIMEOUT
} error_t;

// helper functions
extern void delay_ms(double ms);
extern error_t error_handle(error_t err);


#endif // INC_COMMON_DEFINES_H