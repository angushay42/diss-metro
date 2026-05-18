#include "common-defines.h"
#include "dependencies.h"
#include "duart.h"
#include "dmetronome.h"
#include "dspi.h"
#include "dfft.h"
#include "dsystime.h"
#include "ddetect.h"

#include <libopencm3/stm32/f4/usart.h>

static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

extern void delay_ms(double ms) {
    size_t i, n;

    // 7 gives 99 ms 
    for (i = 0, n = (size_t) round(ms * CPU_FREQ / 1000 / 7); i < n; i++)
        __asm volatile ("NOP");
}


extern error_t error_handle(error_t err) {
    // disable interrupts
    duart_teardown();
    dspi_teardown();
    dmetro_teardown();
    sys_teardown();

    
    // loop forever and toggle the ERROR LED with the code
    while (1) {
        for (size_t i = 0; i < (size_t) err; i++) {
            gpio_set(ERROR_LED_PORT, ERROR_LED_PIN);
            delay_ms(750);
            gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
            delay_ms(750);
        }
        delay_ms(5000); // 5 seconds
    }
    // should never reach
    return err;
}


uint64_t detect_arr[2];
struct packet detected_packet = {
    .is_signed = false,
    .id = "DETECT",
    .len = 2,
    .size = sizeof(uint64_t),
    .u = detect_arr
};

static void report_results(uint64_t note_stamp, bool ans) {
    detect_arr[0] = note_stamp;
    detect_arr[1] = (uint64_t) ans;
    duart_send_packet(&detected_packet);
}

uint64_t stamp;
short sample;
struct packet stampp = {
    .id = "STAMP",
    .u = &stamp,
    .len = 1,
    .size = sizeof(stamp),
    .is_signed = false,
},
samplep = {
    .id = "SAMPLE",
    .u = &sample,
    .len = 1,
    .size = sizeof(sample),
    .is_signed = true
};

static void send_stamped_sample(void) {
    // get one sample
    dspi_rcv(&sample);
    
    // get a stamp
    stamp = get_time(false);

    // // send both
    // duart_send_packet(&stampp);
    // duart_send_packet(&samplep);
}

#define TEST_LED_PORT (GPIOB)
#define TEST_LED_PIN (GPIO6)

double start_freq;
uint32_t bpm;
uint32_t pulse_period;
uint32_t pulse_psc;
uint32_t bpm_psc;

static error_t minimal_timer_setup(void) {
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_clock_enable(RCC_GPIOB);
    nvic_enable_irq(NVIC_TIM3_IRQ);


    gpio_mode_setup(TEST_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, TEST_LED_PIN);
    // set to 0 to start with
    gpio_clear(TEST_LED_PORT, TEST_LED_PIN);

    timer_disable_counter(TIM3);
    timer_set_mode(
        TIM3,
        TIM_CR1_CKD_CK_INT_MUL_4,
        TIM_CR1_CMS_EDGE,
        TIM_CR1_DIR_UP
    );

    timer_set_prescaler(TIM3, 65535);

    timer_disable_preload(TIM3);
    timer_continuous_mode(TIM3);

    start_freq = 84000000.0 / 65535.0;
    bpm = 120;
    bpm_psc = (uint32_t) (start_freq / ((double) bpm / 60.0));
    pulse_period = 100;  // ms
    pulse_psc = (uint32_t) (start_freq / ((uint32_t) (1.0 / (pulse_period / 1000.0))));

    // convert bpm to hz ( divide by 60) then divide start freq by desired frequency to get prescale
    timer_set_period(TIM3, bpm_psc);
    // should just be overflow
    timer_enable_irq(TIM3, TIM_DIER_UIE);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);
    timer_enable_counter(TIM3);

    return OK;
}

void tim3_isr(void) {
    if (timer_get_flag(TIM3, TIM_SR_UIF)) {
        // gpio_toggle(TEST_LED_PORT, TEST_LED_PIN);
        gpio_set(TEST_LED_PORT, TEST_LED_PIN);
        
        timer_enable_oc_output(TIM3, TIM_OC1);
        // uint32_t temp = (timer_get_counter(TIM3) + pulse_psc) % bpm_psc;
        timer_set_oc_value(TIM3, TIM_OC1, pulse_psc);

        timer_clear_flag(TIM3, TIM_SR_UIF);
    }
    else if (timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        gpio_clear(TEST_LED_PORT, TEST_LED_PIN);
        timer_disable_oc_output(TIM3, TIM_OC1);
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
    }
}


static error_t minimal_uart_setup(void) {
    /* enable clock */
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_GPIOA);

    /* enable and set up the pins */
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO2 | GPIO3);
    
    /* disable first */
    usart_disable(USART2);
    
    /* config */
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(USART2, 115200);
    usart_set_stopbits(USART2, 1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_databits(USART2, 8);

    /* read DR register to reset rxne flag */
    uint16_t data = USART2_DR;

    /* enable recieve interrupt */
    usart_enable_rx_interrupt(USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);
    
    /* finally enable usart */
    usart_enable(USART2);
    return OK;
}


// int main(void) {
//     rcc_setup();
//     error_t err;
//     rcc_periph_clock_enable(RCC_GPIOC);
//     gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ERROR_LED_PIN);
//     gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);

//     volatile uint16_t data;
//     if ((err = minimal_uart_setup()))
//         return error_handle(err);
//     while (1) {
//         // while (usart_get_flag(USART2, USART_FLAG_RXNE) != 1)
//         //     ;
//         // data = usart_recv(USART2);
//         // usart_send_blocking(USART2, (uint16_t) 'A');
//         // data = usart_recv_blocking(USART2);
//         ; // do nothing
//         while ((USART2_SR & USART_FLAG_RXNE) != 1);
//         data = usart_recv(USART2);
//         if (USART2_SR & USART_FLAG_ORE)
//             error_handle(2);
//         if (USART2_SR & USART_FLAG_FE)
//             error_handle(3);
//     }
// }

int main(void) {
    rcc_setup();
    error_t err;
    
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, ERROR_LED_PIN);
    gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
    
    if ((err = dspi_setup())) {
        error_handle(err);
        return err;
    }
    if ((err = duart_setup())) {
        error_handle(err);
        return err;
    }
    if ((err = dmetro_setup())) {
        error_handle(err);
        return err;
    }

    if ((err = sys_setup(10)))
        return error_handle(err);
    
    nvic_set_priority(NVIC_TIM4_IRQ, 2);
    nvic_set_priority(NVIC_USART2_IRQ, 1);
    
    size_t sample_idx, sample_size, max_size;
    max_size = 64;
    uint64_t note_stamp, next_beat, now, listening_period;
    short samples[max_size], sample;
    bool ans;

    listening_period = 50; // ms
    sample_size = 10;       

    while (1) {
        // dspi_rcv(&sample);
        // send_stamped_sample();

        // duart_send_packet(&samples_pack);
        // *stamps = get_time(false);
        // dmetro_poll_update((uint64_t) 250); 
        ;
    }
    return 0;
}
