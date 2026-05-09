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

    if ((err = test_handle(&test_duart, "DUART PROTOCOL")))
        return err;

    print_line(30);
    printf("Ran %i test%s \n", test_count, (test_count > 1) ? "s": "");
    printf("OK\n");
    print_line(30);

    return 0;
}