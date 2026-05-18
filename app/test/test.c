#include "test.h"




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

void print_bits(uint8_t b) {
    uint8_t vals[8];
    for (size_t i = 0; i < 8; i++) {
        vals[i] = (b & 1);
        b >>= 1;
    }

    for (size_t i = 8; i > 0; i--) {
        printf("%u", vals[i-1]);
    }
    printf("\n");
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

struct test {
    union {
        uint8_t *bytep;
        uint16_t *halfp;
        uint32_t *wordp;
        uint64_t *doublep;
        double *floatp;
    };
};

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
    
    /* create array of bytes */
    uint8_t arr[256];

    /* store 4 bytes in there */
    arr[0] = 0xFF;
    arr[1] = 0x1;
    arr[2] = 0x36;
    arr[3] = 0;

    printf("4 bytes: %u, %u, %u, %u\n", arr[0], arr[1], arr[2], arr[3]);
    printf("One 32bit word: %llu\n", *((uint32_t*) arr));

    uint32_t arr2[256];

    arr2[0] = (uint32_t) 0xFF013600;

    printf("one 32 bit word: %u\n", arr2[0]);
    printf("4 bytes: %u, %u, %u, %u\n", *((uint8_t*) arr2), *((uint8_t*) arr2 + 1), *((uint8_t*) arr2 + 2), *((uint8_t*) arr2 + 3));


    


    if ((err = test_handle(&test_duart, "DUART PROTOCOL")))
        return err;

    if ((err = test_handle(&test_stack, "STACK")))
        return err;

    print_line(30);
    printf("Ran %i test%s \n", test_count, (test_count > 1) ? "s": "");
    printf("OK\n");
    print_line(30);

    return 0;
}