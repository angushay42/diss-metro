#include "test.h"
#include <ctype.h>

#define UART 1
#define OUTPUT_SIZE 64
#define MIN_PACKET  (5)
#define MAX_PACKET  (4 + (255 * 64))


struct packet {
    char *id;
    /* union means each member occupies the same memory space, so it can be accessed depending on how you use it */
    union {
        double *f; 
        void *u;
    };
    size_t size;
    size_t len;
    bool is_signed;
};


char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
char ans[3];

void byte_to_hex(uint8_t byte) {
    uint8_t first, second;
    first = byte / 16;
    second = byte % 16;
    ans[0] = map[first];
    ans[1] = map[second];
    ans[2] = '\0';
}

/**
 * Acts like Python. If byte can be represented as alphanumeric/special, it will;
 * otherwise, hex will be printed.
 * sends to std output
 */
void usart_send_blocking(uint32_t usart, uint16_t d) {
    char c = (char) d;
    // 
    if (isalnum(c) || c == '{' || c == '}') {   // alphanumeric 
        printf("%c%s", c, (c == '}') ? "\n": "");
        
        return;
    }
    byte_to_hex((uint8_t) c);
    printf("\\x%s", ans);
}

int test_mock() {
    for (size_t i = 0; i < 256; i++) {
        byte_to_hex((uint8_t) i);
        printf("%s\n", ans);}
    return 0;
}


static error_t duart_send_8(struct packet *p) {
    uint8_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        usart_send_blocking(UART, (uint8_t) temp);
    }
    return OK;
}

static error_t duart_send_16(struct packet *p) {
    uint16_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

static error_t duart_send_32(struct packet *p) {
    uint32_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

static error_t duart_send_64(struct packet *p) {
    uint64_t *ptr, temp;
    ptr = (*p).u;
    for (size_t i = 0; i < (*p).len; i++) {
        temp = *(ptr + i);
        for (size_t j = 0; j < (*p).size; j++) {
            usart_send_blocking(UART, (uint8_t) temp);
            temp >>= 8;
        }
    }
    return OK;
}

// todo wish this was more general...
extern error_t duart_send(struct packet *p) {
    if (p == NULL)
        return DUART_SEND_NULL;

    error_t err;
    uint8_t flag;

    // send start char
    usart_send_blocking(UART, (uint8_t) '{');

    flag = (1 << ((*p).size / 2));
    if ((*p).is_signed)
        flag |= (1 << fsigned);

    // send metadata in flag
    usart_send_blocking(UART, (uint8_t) flag);

    // send length byte
    usart_send_blocking(UART, (uint8_t) (*p).len);

    switch ((*p).size) {
        case 1:
            duart_send_8(p);
            break;
        case 2:
            duart_send_16(p);
            break;
        case 4:
            duart_send_32(p);
            break;
        case 8:
            duart_send_64(p);
            break;
        default:
            return DUART_INVALID_SIZE;
    }
    
    // send stop char
    usart_send_blocking(UART, (uint8_t) '}');


    return OK;
}


extern error_t duart_send_packet(struct packet *p) {
    error_t err;
    size_t n;

    if ((*p).id == NULL)
        return DUART_SEND_NULL;
    
    /** create packet for string identifier and send it */
    /* get length of id */
    for (n = 0; (*p).id[n]!= '\0'; n++)
        ;
        if (n > (size_t) 255)
        return DUART_INVALID_SIZE;
        
    // todo chek n
    
    struct packet ch = {
        .len = n,
        .size = 1,
        .is_signed = false,
        .u = (*p).id,
    };

    // send id
    if ((err = duart_send(&ch)))
        return err;

    // send data
    if ((err = duart_send(p)))
        return err;
    return OK;
}


extern int test_duart() {
    // test_mock();

    printf("\\%s", "\n");

    error_t err;
    if (!(err = duart_send(NULL)))
        return 1;

    //* Test send  */
    /* Test floats */
    size_t size, len;
    double nums[] = {0.2239, 190.254, 29030.0, 56.28, 90.143};
    
    len = 5;
    size = sizeof(nums[0]);
    
    struct packet test_packet = {
        .size = size,
        .len = len,
        .f = nums,
        .is_signed = false
    };

    duart_send(&test_packet);

    
    //* test send packet */
    /* Test signed shorts (samples) */
    short samples[] = {-43, 590, 0, -390, 1010, 2};
    size = sizeof(short);
    len = 6;
    
    struct packet test_sample = {
        .id = "SAMPLES",
        .size = size,
        .len = len,
        .u = samples,
        .is_signed = true
    };
    

    duart_send_packet(&test_sample);
    return 0;
}
