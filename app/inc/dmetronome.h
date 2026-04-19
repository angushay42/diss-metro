#ifndef DMETRONOME_H
#define DMETRONOME_H

#include <math.h>
#include "common-defines.h"

#ifndef TESTING
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/adc.h>
#include "dependencies.h"
#endif 


#define MAX_BPM     (220)
#define MIN_BPM     (40)
#define BPM_START   ((uint16_t) 120)
#define MAX_PSC     (uint32_t)((1 << 16) - 1)

#define METRONOME_CH1_PORT  (GPIOB)
#define METRONOME_CH1_PIN   (GPIO6)


extern error_t dadc_setup(void);
extern error_t dmetro_get_tempo_reading(uint16_t *data);
extern error_t dmetro_get_volume_reading(uint16_t *data);

extern error_t dmetro_set_tempo(uint16_t bpm);
extern uint16_t dmetro_get_tempo(void);
extern void dmetro_start(void);
extern void dmetro_stop(void);
extern error_t dmetro_setup(void);

#endif  // DMETRONOME_H