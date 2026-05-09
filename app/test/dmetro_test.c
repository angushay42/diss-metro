#include "test.h"

// tempo mockup vars
uint8_t eoc_flag;
uint16_t fake_readings[] = {3023, 40, 0, 4005, 450, 1000, 2000, 3131};
uint16_t fake_answers[] = {173, 42, 40, 216, 60, 84, 128, 178};
size_t len_fake_readings = 8, fake_idx = 0;


// dmetro global variables
uint32_t _bpm, _psc, rcc_apb1_frequency = 42000000;
uint16_t MAX_PSC = (uint16_t) ((1 << 16) - 1);

/* Set bpm of metronome. Uses defined min and max bpm */
extern error_t dmetro_set_tempo(uint16_t bpm) {
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return DMETRO_INVALID_TEMPO;   // error
    
        
    // input / output = psc
    _psc = (uint32_t) roundf((float) (((float) rcc_apb1_frequency / (float) MAX_PSC) / ((float) bpm / 60.0)));
    _bpm = bpm;

    // timer_set_period(TIM4, _psc - 1);
    return OK;
}

static uint16_t scale_to_bpm(uint16_t reading) {
    float temp = (float) reading;
    uint16_t res;

    // https://stats.stackexchange.com/questions/281162/scale-a-number-between-a-range
    // m = ((m - r_min) / (r_max - r_min)) * (t_max - t_min) + t_min
    temp = temp / (float) ((1 << 12) - 1);
    temp = temp * (float) (MAX_BPM - MIN_BPM);
    temp = temp + (float) MIN_BPM;

    // original scale is 0-4096 (1<<12)
    // temp = ((temp - 0) / (float) (1 << 12)) * (MAX_BPM - MIN_BPM) + MIN_BPM;
    // scale reading into total range

    res = (uint16_t) roundf(temp);
    // clamp reading
    if (res > MAX_BPM)
        res = MAX_BPM;
    else if (res < MIN_BPM)
        res = MIN_BPM;
    return res;
}

// TODO should ensure psc is valid range 0<=psc<maxpsc
static error_t convert_to_psc(uint16_t bpm) {
    double input, output, psc;
    uint32_t temp;
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return DMETRO_INVALID_TEMPO;

    // input / output = psc
    input = (double) rcc_apb1_frequency / (double) MAX_PSC;
    output = (double) bpm / 60.0;
    psc = roundf(input / output);
    temp = (uint32_t) psc;
    if (temp >= MAX_PSC)
        return DMETRO_INVALID_PSC;
    _psc = (uint32_t) psc;
    return OK;
}


uint16_t adc_read_regular() {
    return fake_readings[(fake_idx++) % len_fake_readings];
}

extern error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout) {
    uint32_t i;
    
    for (i = 0; !(eoc_flag); i++)
        if (cycle_timeout > 0 && i > cycle_timeout) {
            *data = 1 << 14;
            return DADC_TIMEOUT;
        }

    *data = scale_to_bpm(adc_read_regular());

    return OK;
}

extern int test_dmetro() {
    size_t i;
    error_t err;
    uint16_t temp, timeout;

    // test bpm scaling    
    for (i = 0; i < len_fake_readings; i++) {
        temp = scale_to_bpm(fake_readings[i]);
        // printf("expected: %u, actual: %u\n", temp, fake_answers[i]); //todo remove
        if (temp != fake_answers[i]) {
            printf("input: %zu, output: %u\n", i, temp);
            return 1;
        }
    }

    // test psc conversion
    uint16_t bad_bpms[] = {1, 32, 20000, 300};
    for (i = 0; i < 4; i++)
        if ((err = convert_to_psc(bad_bpms[i]))!= DMETRO_INVALID_TEMPO) {
            printf("bad input: %u\n", bad_bpms[i]);
            return 2;
        }
    uint16_t input_bpms[] = {120, 45, 173, 180, 68, 90};
    uint16_t output_bpms[] = {320, 855, 222, 214, 565, 427};

    for (i = 0; i < 5; i ++) {
        if ((err = convert_to_psc(input_bpms[i]))) {
            printf("error: %u, input bpm: %u, output_bpm: %u, output_psc: %u\n", err, input_bpms[i], _bpm, _psc);
            return 3;
        }
        if (_psc != output_bpms[i]) {
            printf("input bpm: %u, output_bpm: %u, output_psc: %u\n", input_bpms[i], _bpm, _psc);
            return 4;
        }
    }

    // check ALL bpm versions
    for (i = (size_t) MIN_BPM; i < (size_t) MAX_BPM; i++) {
        if ((err = convert_to_psc((uint16_t) i))) {
            // printf("bpm: %zu, err: %u\n", i, err);
            return 5;
        }
        printf("bpm: %zu, psc: %u\n", i, _psc);
            
    }

    // test timeout
    eoc_flag = 0;       // mock conversion never completing
    timeout = 40000;
    if ((err = dmetro_get_tempo_reading(&temp, timeout)) != DADC_TIMEOUT
        || temp != (1 << 14)) {
        return 5;
    }

    eoc_flag = 1;
    if ((err = dmetro_get_tempo_reading(&temp, 0)) != OK
        || temp != fake_answers[fake_idx-1]) {
        return 6;
    }

    return 0;
}
