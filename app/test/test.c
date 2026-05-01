#include "test.h"

//======================= prototypes =======================
int test_ring_buffer();
int test_fft();
int test_split();
int test_bit_reversal(uint32_t test[], uint32_t res[], uint32_t size);
int test_delay();
int test_error_handle();

// helpers
void print_complex(char *s, complex_t x);
void print_line(size_t len);
void confirm_called(char *s);

// copied functions
uint16_t bit_reverse(uint16_t x, uint32_t lg2n);
uint32_t int_log2(uint32_t x);
static void delay_cycles(size_t cycles);
static void delay_ms(double ms);
static error_t error_handle(error_t err, time_t timeout, long count);
extern error_t dmetro_set_tempo(uint16_t bpm);
static uint16_t scale_to_bpm(uint16_t reading);
static error_t convert_to_psc(uint16_t bpm);
uint16_t adc_read_regular();
extern error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout);
// ---------------------- endprototypes ----------------------


// ======================= helper functions =======================
void print_line(size_t len) {
    for (size_t i = 0; i < len; i++)
        printf("=");
    printf("\n");
}

void print_complex(char *s, complex_t z) {
    printf("%s = %.1f%+.1fi\n", s, creal(z), cimag(z));
}

void confirm_called(char *s) {
    print_line(40);
    printf("called: %s\n", s);
    print_line(40);
}
// ---------------------- end helper ----------------------


// ======================= copied functions ===========================

short convert(uint16_t input) {
    short output;
    if (!((1 << 12) & input)) {
        printf("positive\n");
        return output = input;
    }
    printf("negative\n");
    output = (((uint16_t) (15 << 12)) | input);
    return output;
}

/****************** fft functions *****************/

/* reverse the bits in unsigned integer x */
uint16_t bit_reverse(uint16_t x, uint32_t lg2n) {
    // reverse the bits in a word
    uint16_t r;
    r = 0;
    for (uint16_t i = 0; i < lg2n; i++) {
        // extract bit from right to left and build r from left to right
        r = (r << 1) | (x & 1);
        x >>= 1;
    }
    return r;
}

/* simple unsigned int log2 function */
uint32_t int_log2(uint32_t x) {
    uint32_t log = 0;
    // divide by 2 until less than 0
    while (x >>= 1) {log++;}
    return log;
}
/****************** end fft *****************/


/***************** delay functions ************************/
static void delay_cycles(size_t cycles) {
    confirm_called("delay_cycles");
    printf("cycles: %zu\n", cycles);

    for (size_t i = 0; i < cycles; i++)
        __asm volatile ("NOP");
}

static void delay_ms(double ms) {
    confirm_called("delay_ms");
    long i, n;

    // Mac has ~2.7GHz frequency
    // so 2_700_000_000 / 1000 = 2_700_000 or 2700000.0
    // 13 clock cycles (roughly the cycles used for the function in test/scrap)
    for (i = 0, n = (long) round(ms * 2700000.0 / 13.0); i < n; i++) {
        __asm volatile ("NOP");
    }
}
/***************** end delay ************************/


/******************* error handle ********************/
// error handle vars
int test_led_on, test_led_off;

static error_t error_handle(error_t err, time_t timeout, long count) {
    time_t now, start = time(NULL);
    long i = 0;
    
    test_led_on = test_led_off = 0;

    // loop forever and toggle the ERROR LED with the code
    while (1) {
        now = time(NULL);
        if (timeout > (time_t) 0 && (now - start) >= timeout) {
            err = ERROR_HANDLE_TIMEOUT;
            break;
        }
        if (i >= count) {
            err = ERROR_HANDLE_TIMEOUT;
            break;
        }
        for (size_t j = 0; j < (size_t) err; j++) {
            //todo swap
            test_led_on++;
            printf("LED ON\n");
            delay_ms(500);
            test_led_off++;
            printf("LED OFF\n");
        }
        delay_ms(2500);
        i++;
    }
    // should never reach
    return err;
}
/******************** end error handle *********************/


/************************ set tempo *********************/
// tempo vars
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

// tempo mockup vars
uint8_t eoc_flag;
uint16_t fake_readings[] = {3023, 40, 0, 4005, 450, 1000, 2000, 3131};
uint16_t fake_answers[] = {173, 42, 40, 216, 60, 84, 128, 178};
size_t len_fake_readings = 8, fake_idx = 0;

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
/********************** end set tempo ***************/



/************************************ sys time ********************************************************* */

/* time in ms */
volatile uint64_t sys_time;
uint32_t _resolution, _sys_psc;

/* tick time in ms*/
double scale_value;

void sys_tick_handler(void) {
    uint64_t temp64 = sys_time;
    sys_time = temp64 + _resolution;
}


/* resolution in ms */
error_t sys_setup(uint32_t resolution) {
    // systick should have 10.5MHz (84/8) 
    double input, output;
    uint32_t psc;

    // todo some check for res
    _resolution = resolution;
    input = 10500000.0;
    output = 1 / ( (double) resolution / 1000.0);
    psc = (uint32_t) roundf(input / output);
    if (psc > ((1 << 25)- 1))
        return SYSTICK_INVALID_RESOLUTION;
    _sys_psc = psc;
    scale_value = 1000.0 / input;   // in ms

    sys_time = 0;
    return OK;
}

static uint32_t systick_get_value(void) {
    return (uint32_t) 3000;
}

/* get time in ms*/
static uint64_t get_time() {
    uint64_t temp64 = 0;
    // (reload - curr) = ticks counted
    // 1 tick = 1/10.5mhz = 0.000000095238095 seconds
    temp64 = (_psc - systick_get_value()) * scale_value;
    return sys_time + temp64;
}

/************************************ end sys time ********************************************************* */
/************************************ start uart protocol ********************************************************* */

size_t temp;

extern error_t duart_start_sequence(void *data) {
    if (data == NULL)
        return DUART_START_SEQUENCE;
    
    temp = sizeof(*data);
    return OK;
}

/************************************ end uart protocol ********************************************************* */



// ----------------------- end copied -------------------------


// ========================= test functions =========================

int test_duart_protocol() {
    error_t err;
    if (duart_start_sequence(NULL) != DUART_START_SEQUENCE)
        return 1;

    // check sizes
    uint8_t byte = 0;
    uint16_t half = 0;
    uint32_t word = 0;
    uint64_t double_word = 0;

    if (duart_start_sequence((void *) (&byte))) {
        return 2;
    }
    printf("temp: %zu\n", temp);

    if (duart_start_sequence((void *) (&half))) {
        return 3;
    }
    printf("temp: %zu\n", temp);

    if (duart_start_sequence((void *) (&word))) {
        return 4;
    }
    printf("temp: %zu\n", temp);

    if (duart_start_sequence((void *) (&double_word))) {
        return 5;
    }
    printf("temp: %zu\n", temp);
    return 0;
}

int test_sys_time() {
    //todo
    error_t err;
    uint64_t start, end, temp;


    if ((err = sys_setup(1000)))
        return 1;

    if ((err = sys_setup(10)))
        return 2;

    start = get_time();
    sys_tick_handler();
    temp = get_time();
    if (temp - start != 10)
        return 3;
    

    // printf("%u, %u, %f\n", _sys_psc, _resolution, scale_value);
    printf("get_time: %llu\n", get_time());
    return 0;
}

int test_average_samples() {
    size_t i, n= 20;
    uint16_t samples[n];
    for (i = 0; i < n; i++)
        samples[i] = i * 139;
    
    return 0;
}

int test_error_handle() {
    int err;
    long count;
    error_t input_err, return_err;
    time_t timeout, thresh, actual, expected;
    struct timespec start, end;

    /* test timeout*/
    timeout = 1, count = -1;
    if ((return_err = error_handle(1, timeout, count)) != ERROR_HANDLE_TIMEOUT)
        return (err = 1); 

    /* test count param */
    timeout = -1, count = 1;
    if ((return_err = error_handle(1, timeout, count)) != ERROR_HANDLE_TIMEOUT)
        return (err = 2); 

    /* test with actual errors */
    input_err = MAIN_LOOP, timeout = -1, count = 1;
    if ((return_err = error_handle(input_err, timeout, count)) != ERROR_HANDLE_TIMEOUT 
        && (test_led_off != (int) input_err || test_led_on != (int) input_err)
    )
        return (err = 3); 

    input_err = DFFT_INVALID_ARGS, timeout = -1, count = 1;
    if ((return_err = error_handle(input_err, timeout, count)) != ERROR_HANDLE_TIMEOUT 
        && (test_led_off != (int) input_err || test_led_on != (int) input_err)
    )
        return (err = 4); 

    /* test total time taken */
    input_err = DFFT_INVALID_N, timeout = -1, count = 1;
    thresh = 5000;  // ns
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    return_err = error_handle(input_err, timeout, count);
    clock_gettime(CLOCK_MONOTONIC, &end);

    actual = end.tv_nsec - start.tv_nsec;
    // delays 2.5 seconds per count
    // delays 0.5 seconds per blink
    expected = ((time_t)( 2.5 * 1000000)* count) + ((time_t)( 0.5 * 1000000 * input_err));
    
    if ((actual - expected) > thresh) {
        printf("diff in nanoseconds: %ld\n", end.tv_nsec - start.tv_nsec);
        return (err = 4); 
    }


    return (err = 0);
}

int test_delay() {
    /* compiler only shows */
    int err;
    double ms;
    struct timespec start, end;
    time_t thresh, diff;

    clock_gettime(CLOCK_MONOTONIC, &start);
    delay_ms((double) (ms = 1000));
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    diff = (time_t) round(end.tv_nsec-start.tv_nsec);
    thresh = 500; // nanosec
    if ((time_t) fabs(diff - (ms * 1000.0)) <= thresh)
    if (round(diff) != (ms / 1000.0)){
        printf("difference: %ld\n", diff);
        return 1;
    }
    return 0;
}

int test_tempo_conversion() {
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

//todo update signature to match 
int test_bit_reversal(uint32_t test[], uint32_t res[], uint32_t size) {
    size_t i;
    int err;

    for (i = 0; i < size; i++) {
        test[i] = bit_reverse((uint16_t) i, int_log2((uint32_t) size));
    }

    err = 0;
    for (i = 0; i < size; i++) {
        err |= !(test[i] == res[i]);
    }
    return err;
}

int test_fft() {
    uint32_t size;
    uint16_t test;
    complex_t res1, res2;

    short test2;
    size_t i, err;  //todo why size_t?

    test = (uint16_t) -432;
    test2 = -432;
    size = 8;

    if ( (res1 = (complex_t) test) == (res2 = (complex_t) test2))
        return 1;
    
    uint32_t test_arr[16];
    uint32_t first_res[] = {0, 4, 2, 6, 1, 5, 3, 7};
    uint32_t second_res[] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};
    
    // test N=8
    if ((err = test_bit_reversal(test_arr, first_res, (uint32_t) 8)))
        return 2;
    
    // test N=16
    if ((err = test_bit_reversal(test_arr, second_res, (uint32_t) 16)))
        return 3;
    
    

    // the dft will give us N sample frequencies to analyse with
    // the dft is a summation with some complex sinusoid (Lyons, R.G.)
    // that sinusoid has integral frequencies at n*(f_s / N) 
    // so that would be 44100 / 16 = ~2,756.25hz
    // the dft outputs complex numbers, the magnitude of which is given by:
    // |X(m)| = sqrt( x_real(m)^2 + X_imag(m)^2) )
    // data collection time interval is the reciprocal of the desired resolution (Lyons, R.G., pp138)
    // resolution of dft is the distance from sample to sample in hz
    // N = fs / desired
    // f_analysis(m) = m*fs / N


    double *test_sin, temp, fs, desired;
    desired = 10;    // hz
    fs = 44100.0;   // should this be the same as the ADC?

    // must be power of 2!!
    // must be at least fs / desired, so take the ceiling of the double. 
    size = 1 << (uint32_t) ceil(log2(fs / desired));

    test_sin = (double *) malloc(sizeof(double) * size);
    if (test_sin == NULL)
        return -1;

    print_line(40);
    printf("testing FFT with params:\n");
    printf("\tdesired resolution: %f\n", desired);
    printf("\tfs: %f\n", fs);
    printf("\tN: %u\n\n", size);

    // sine wave sampled at 44100Hz
    printf("generating sine wave...\n");
    // fill sine wave
    // printf("sine values: {");
    for (i = 0; i < size; i++) {
        temp = sin(2.0 * PI * 440.0 * (double) i / fs);
        // printf("%f%s", temp, (i == size-1) ? "": ", ");
        test_sin[i] = temp;
    }
    // printf("}\n");
    printf("sine wave generated\n");
    
    
    complex_t stack_res[size];
    printf("testing stack fft...\n");
    // test stack
    fft(test_sin, stack_res, size);
    printf("stack fft tested\n");

    double mag, spike;
    size_t idx;

    spike  = -1;
    idx = -1;

    for (i = 0; i < size; i++) {
        mag = sqrt(pow(creal(stack_res[i]), 2) + pow(cimag(stack_res[i]), 2));
        if (mag > spike) {
            spike = mag;
            idx = i; 
        }
        // printf("frequency domain at [%zu]: %f\n", i, creal(stack_res[i]));
    }
    printf("|X[%zu]| = %f. Freq: %f\n", idx, spike, idx * fs / size);
    if (stack_res[0] != 1) {
        free(test_sin);
        return 4;
    }
    
    complex_t *heap_res = malloc((size_t) (sizeof(complex_t) * size));
    if (heap_res == NULL)
        return 5;

    printf("testing heap fft...\n");
    // test heap
    fft(test_sin, heap_res, size);
    printf("heap fft tested\n");
    if (heap_res[0] != 1) {  // unity amplitude (1)
        free(heap_res);
        return 6;
    }
    free(test_sin);
    free(heap_res);

    return (err = 0);
}

// todo expand this more
int test_split() {
    int err;
    uint16_t data;

    data = 1097;
    printf("first half (LSB) %u\n", (uint8_t) data);
    printf("first half (LSB) %u\n", (uint8_t) (data >> 8));
    return (err = 0);
}

int test_ring_buffer() {
    ring_buf_t rb;
    uint16_t word, data, buffer[RING_BUF_MAX];
    int err;

    if ((err = dring_buf_setup(&rb, buffer, RING_BUF_MAX))) {
        return 1;
    }

    // write a word to buffer
    if ((err = dring_buf_write(&rb, (word = 1097))))
        return 2;

    // read the only word present
    if ((err = dring_buf_read(&rb, &data))|| data != 1097)
        return 3;

    // buffer should be empty
    if (!dring_buf_empty(&rb)) 
        return 4;

    // write a word and check if empty
    if ((err = dring_buf_write(&rb, word)) && !dring_buf_empty(&rb))
        return 5;
    
    return 0;
}

static int test_count = 0;

int test_handle(int (*test_f)(), char *s) {
    int err;
    test_count++;
    if ((err = test_f())){
        printf("%s FAIL with code: %i\n", s, err);
        return test_count;
    }
    return 0;
}

/**************************** main ************************/
int main(void) {
    int err;

    // if (test_handle(&test_fft, "FFT"))
    //     return 1;

    // if ((err = test_handle(&test_tempo_conversion, "TEMPO CONVERSION")))
    //     return err;
    
    // if ((err =test_handle(&test_delay, "DELAY")))
    //     return err;
    
    // if ((err = test_handle(&test_error_handle, "ERROR HANDLE")))
    //     return err;

    // if ((err = test_handle(&test_sys_time, "SYS TIME")))
    //     return err;

    // if ((err = test_handle(&test_duart_protocol, "DUART PROTOCOL")))
    //     return err;

    double test;
    test = 101.3202;
    printf("%f, %llu\n", test, (uint64_t) test);
    // for (size_t i = 0; i < sizeof(double) * 8; i++)
    //     printf("%i\n", test & 1)


    print_line(30);
    printf("Ran %i test%s \n", test_count, (test_count > 1) ? "s": "");
    printf("OK\n");
    print_line(30);

    return 0;
}