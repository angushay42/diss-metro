#include <stddef.h>
#include <stdio.h>
#include <math.h>


int convert(__uint16_t input) {
    __uint16_t mask, mask2, mask3;
    int converted;
    mask = 1 << 12;
    mask2 = (__uint16_t) ~( 15 << 12);
    mask3 = (__uint16_t) ~(7 << 12);
    // keep 4msb as 0
    if (!(mask & input))
        return (int) (mask2 & input); 
    return (int) (mask3 & input);
}

short convert2(__uint16_t input) {
    short output;
    if (!((1 << 12) & input)) {
        printf("positive\n");
        return output = input;
    }
    printf("negative\n");
    output = (((__uint16_t) (15 << 12)) | input);
    return output;
}

int main(void) {
    __uint16_t temp, pos_temp, neg_temp, mask;
    mask = (1) << 12;
    pos_temp = 0b0000111111111111;
    neg_temp = 0b0011110011111111;

    // printf("%i\n", ~((__uint16_t) ~(15 << 12)));
    // printf("%i\n", (short) ~(((__uint16_t) ~(15 << 12)) & neg_temp));
    printf("%i\n", (__uint16_t) -1);
    printf("casted pos: %i, neg: %i\n", (int) pos_temp, (int) neg_temp);
    printf("converted pos: %i, neg: %i\n", convert2(pos_temp), convert2(neg_temp));

    // printf("test: %i\n", (__uint8_t) ~((__uint8_t) 7 << 5));
    
    return 0;
}