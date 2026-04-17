#ifndef METRONOME_H
#define METRONOME_H


#include <libopencm3/stm32/timer.h>
#include <math.h>
#include "common-defines.h"
#include "dependencies.h"


#define MAX_BPM     (220)
#define MIN_BPM     (40)
#define BPM_START   (120)
#define MAX_PSC     (uint32_t)((1 << 16) - 1)

#define METRONOME_CH1_PORT  (GPIOB)
#define METRONOME_CH1_PIN   (GPIO6)


extern int metro_set_tempo(uint32_t bpm);
extern uint32_t metro_get_tempo(void);
extern void metro_start(void);
extern void metro_stop(void);
extern int metro_setup(void);
// extern void tim4_isr(void); //todo needed?

#endif  // METRONOME_H