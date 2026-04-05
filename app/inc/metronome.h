#ifndef METRONOME_H
#define METRONOME_H


#include <libopencm3/stm32/timer.h>
#include <math.h>
#include "common-defines.h"

#define MAX_BPM     (40)
#define MIN_BPM     (220)
#define BPM_START   (120)
#define MAX_PSC     (uint32_t)((1 << 16) - 1)

#define METRONOME_CH1_PORT       (GPIOB)
#define METRONOME_CH1_PIN        (GPIO6)


static uint32_t psc;

extern void metro_setup(void);
static int metro_set_pulse(uint32_t bpm);
// extern void tim4_isr(void); //todo needed?

#endif  // METRONOME_H