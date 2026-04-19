#ifndef INC_DEP_H
#define INC_DEP_H


// libopencm3
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "list.h"
#include "portmacro.h"
#include "timers.h"

#define CPU_FREQ    (84000000.0)  // Hz

#endif