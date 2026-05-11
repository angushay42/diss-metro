#ifndef DSYSTIME_H
#define DSYSTIME_H

#include "common-defines.h"
#include "dependencies.h"

#include <math.h>
#include <libopencm3/cm3/systick.h>

error_t sys_setup(uint32_t resolution);
void sys_teardown(void);
extern uint64_t get_time(bool precise);

#endif // DSYSTIME_H