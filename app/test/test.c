#include "test.h"
/*************** prototypes ****************/
int test_ring_buffer();
int test_fft();
int test_split();
void print_complex(char *s, complex_t x);
int test_bit_reversal(uint32_t test[], uint32_t res[], uint32_t size);
void print_line(size_t len);
/*************** endprototypes ****************/

/**************** helper functions ************/
void print_line(size_t len) {
    for (size_t i = 0; i < len; i++)
        printf("=");
    printf("\n");
}

void print_complex(char *s, complex_t z) {
    printf("%s = %.1f%+.1fi\n", s, creal(z), cimag(z));
}


/**************** copied functions ************/

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

    res = (uint16_t) temp;
    // clamp reading
    if (res > MAX_BPM)
        res = MAX_BPM;
    else if (res < MIN_BPM)
        res = MIN_BPM;
    return res;
}


static void delay_cycles(uint32_t delay_cycles) {
    for (uint32_t i = 0; i < delay_cycles; i++)
        __asm volatile ("NOP");
}

// todo
static void delay_ms(double ms) {
    double tick_to_ms = 1.0 / 84000.0;
    delay_cycles((ms * tick_to_ms) / 4);
}


/**************** test functions ************/

int test_delay() {
    int err;
    double ms;
    time_t start, end;

    start = time(NULL);
    delay_ms((ms = 1000));
    end = time(NULL);
    if (round(end - start) != ms)
        return 1;
    
}

int test_tempo_conversion() {
    size_t i;
    for (i = 0; i < 80 - 1; i++) {
        printf("input: %zu, output: %u\n", i, scale_to_bpm( (uint16_t) i));
    }
    return 0;
}

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
    if ((err = test_f())){
        printf("%s FAIL with code: %i\n", s, err);
        return ++test_count;
    }
    return 0;
}

/**************************** main ************************/
int main(void) {
    int err;
    // int (*tests)[] = {test_ring_buffer, }

    // if (test_handle(&test_fft, "FFT"))
    //     return 1;

    if (test_handle(&test_tempo_conversion, "TEMPO CONVERSION"))
        return 2;
    
    if (test_handle(&test_delay, "DELAY"))
        return 3;
    
    return 0;
}