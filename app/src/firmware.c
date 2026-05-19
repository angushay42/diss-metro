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

    // send both
    duart_send_packet(&stampp);
    duart_send_packet(&samplep);
}

uint8_t command_buf[256];
struct packet commandp = {
    .id = "COMMAND",
    .size = 1,
    .len = 256U,
    .u = command_buf,
    .is_signed = false
};

void queue_toggle(void);
static error_t set_sequence(uint8_t new_sequence[]);
static error_t minimal_set_tempo(uint32_t bpm);
static error_t minimal_timer_setup();
static void tempo_start();
static void tempo_stop();
static void tempo_toggle();


/* beat program */
volatile uint8_t sequence[8] = {0U};
volatile size_t beat_idx = 0;

/* prescaler for timer, set to 2048 by setup */
volatile uint32_t clk_psc = 0;

/* period in clock ticks of the main beat (crotchets in 4/4) */
volatile uint32_t beat_period = 0;

/* period in clock ticks of the LED pulse */
volatile uint32_t pulse_period = 0;

/* period in clock ticks of the half beats (quavers in 4/4) */
volatile uint32_t half_period = 0;  // in clk ticks

/* flag marking the status of the LED */
volatile bool led_status = false;

/* time in ms for the LED to blink flash for */
uint32_t pulse_period_ms = 100;

/* current timer frequency. 84000000 / 2048 = ~41KHz */
double tim_freq;

volatile bool tempo_status = false;
static void tempo_start() {
    /* reset beat index */
    beat_idx = 0;

    led_status = false;

    /* reset counter */
    timer_set_counter(TIM3, 0U);

    /* pend overflow (beat 1) */
    timer_generate_event(TIM3, TIM_EGR_UG);

    /* mark tempo as started */
    tempo_status = true;

    /* start counter */
    timer_enable_counter(TIM3);

    /* enable interrupts */
    nvic_enable_irq(NVIC_TIM3_IRQ);
}
static void tempo_stop() {
    /* disable interrupts */
    nvic_disable_irq(NVIC_TIM3_IRQ);

    /* stop counter */
    timer_disable_counter(TIM3);

    /* mark status of tmepo */
    tempo_status = false;

    /* reset flags */
    timer_clear_flag(TIM3, TIM_SR_UIF);
    timer_clear_flag(TIM3, TIM_SR_CC1IF);
    timer_clear_flag(TIM3, TIM_SR_CC2IF);

    led_status = false;
}

/* push button functionality */
static void tempo_toggle() {
    tempo_status = !tempo_status;
    if (tempo_status)
        tempo_start();
    else 
        tempo_stop();

}

/* add OC value to turn the LED off */
void queue_toggle(void) {

    uint32_t temp;
    /* this works for quavers, but won't for anything more complex. */
    temp = pulse_period + ((beat_idx & 1) ? half_period: 0);

    /* critical section, disable interrupts to toggle shared variable */
    nvic_disable_irq(NVIC_TIM3_IRQ);
    led_status = true;
    nvic_enable_irq(NVIC_TIM3_IRQ);

    /* update new output compare value */
    timer_set_oc_value(TIM3, TIM_OC1, temp);
}

/**
 * overflow = PC9   (beat)
 * oc1      = PB8   (toggle)
 * oc2      = PB9   (quaver)
 */

void tim3_isr(void) {

    if (timer_get_flag(TIM3, TIM_SR_UIF)) {
        /* if beat is set to play, turn LED on*/
        if (sequence[beat_idx] == 1) {    
            gpio_toggle(GPIOC, GPIO9); // todo remove 

            /* turn LED on */
            gpio_set(GPIOB, GPIO6);

            /* queue the timer to turn the LED back off */
            queue_toggle();
        }
        beat_idx = (beat_idx + 1) % 8;
        /* clear flag */
        timer_clear_flag(TIM3, TIM_SR_UIF);
    }
    else if (timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        
        /* if LED is currently on, turn it off*/
        if (led_status) {
            gpio_toggle(GPIOB, GPIO8);  // todo remove 
            /* turn LED off */
            gpio_clear(GPIOB, GPIO6);

            /* critical section */
            nvic_disable_irq(NVIC_TIM3_IRQ);
            led_status = false;
            nvic_enable_irq(NVIC_TIM3_IRQ);
        }
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
    }
    else if (timer_get_flag(TIM3, TIM_SR_CC2IF)) {
        /* if beat is set to play, turn LED on*/
        if (sequence[beat_idx] == 1) {    
            gpio_toggle(GPIOB, GPIO9); // todo remove 

            /* turn LED on */
            gpio_set(GPIOB, GPIO6);

            /* queue the timer to turn the LED back off */
            queue_toggle();
        }
        beat_idx = (beat_idx + 1) % 8;
        /* clear flag */
        timer_clear_flag(TIM3, TIM_SR_CC2IF);
    }

}

static error_t set_sequence(uint8_t new_sequence[]) {
    for (size_t i = 0; i < 8; i++)
        sequence[i] = new_sequence[i];
    return OK;
}

static error_t minimal_set_tempo(uint32_t bpm) {
    if (!(bpm >= MIN_BPM && bpm <= MAX_BPM))
        return 1;
      
    double fbpm = (double) bpm / 60.0;
    uint32_t new_psc = (uint32_t) round(tim_freq / fbpm);
    if (!(new_psc >= 1 && new_psc < MAX_PSC))
        return 2;
        
    /* need to change variables that could be altered in other threads, */
    /* so disable interrupts for this critical section */
    nvic_disable_irq(NVIC_TIM3_IRQ);
    
    /* update if valid */
    beat_period = new_psc;
    timer_set_period(TIM3, beat_period);

    /* set new value for quaver */
    half_period = (uint32_t) round((double) beat_period / 2.0);
    /* use Output Compare channel 2 (OC2) for quavers */
    timer_set_oc_value(TIM3, TIM_OC2, half_period);

    /* reset sequence */
    beat_idx = 0;

    /* exit critical section */
    nvic_enable_irq(NVIC_TIM3_IRQ);

    return OK;
}

static error_t minimal_timer_setup() {
    error_t err;
    /* init global vars */
    clk_psc = 2048U;
    tim_freq = (84000000.0 / (double) clk_psc);  
    pulse_period = (uint32_t) (tim_freq / (1.0 / ((double) pulse_period_ms / 1000.0)));


    /* enable clock */
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_clock_enable(RCC_GPIOB);
    
    /* configure pin */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    gpio_clear(GPIOB, GPIO6);

    /* disable timer first */
    timer_disable_counter(TIM3);

    /* configure */
    timer_continuous_mode(TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_disable_preload(TIM3);

    /* enable output compare on channel 1 and  2*/
    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_oc_output(TIM3, TIM_OC2);

    /* set timer peripheral prescaler */
    timer_set_prescaler(TIM3, clk_psc);

 

    uint8_t seq[] = {1, 1, 1, 1, 1, 0, 1, 0};

    /* set starting tempo */
    if ((err = minimal_set_tempo(120U)))
        return err;
    /* set starting sequence (this will likely be just crotchets)*/
    if ((err = set_sequence(seq)))
        return err;
    /* set toggle pulse */
    timer_set_oc_value(TIM3, TIM_OC1, pulse_period);

    /* enable interrupts */
    nvic_enable_irq(NVIC_TIM3_IRQ);
    timer_enable_irq(TIM3, TIM_DIER_UIE);   // overflow
    timer_enable_irq(TIM3, TIM_DIER_CC1IE); // output compare 1 (LED toggle)
    timer_enable_irq(TIM3, TIM_DIER_CC2IE); // output compare 2 (quaver pulse)

    
    /* finally enable */
    tempo_start();
    return OK;
}

int main(void) {
    error_t err;
    rcc_setup();
    
    /* set up error LED */
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, ERROR_LED_PIN);
    gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);

    if ((err = sys_setup(10U)))
        return error_handle(err);

    if ((err = minimal_timer_setup()))
        return error_handle(err);

    /* 1, 2, 3, 4 */
    uint8_t test1[] = {1, 0, 1, 0, 1, 0, 1, 0};

    /* 1, _, _, _ */
    uint8_t test2[] = {1, 0, 0, 0, 0, 0, 0, 0};

    /* 1 and, 2, _, _ and */
    uint8_t test3[] = {1, 1, 1, 0, 0, 0, 0, 1};

    /* 1 and, 2 and, 3 and, 4 and */
    uint8_t test4[] = {1, 1, 1, 1, 1, 1, 1, 1};

    uint8_t *test_seq[4];
    size_t num_tests, test_idx;
    test_idx = 0;
    num_tests = 4;

    test_seq[0] = test1;
    test_seq[1] = test2;
    test_seq[2] = test3;
    test_seq[3] = test4;

    volatile uint64_t last, now, period;
    last = 0;
    period = 5000; // ms
    while (1) {
        now = get_time(true);
        /* if it is time to poll */
        if (now - last >= period) {
            gpio_set(ERROR_LED_PORT, ERROR_LED_PIN);
            delay_ms(100);
            gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);

            /* update last poll time */
            last = now;

            tempo_stop();
            set_sequence(test_seq[test_idx]);
            delay_ms(500);
            tempo_start();

            /* increment index */
            test_idx = (test_idx + 1) % num_tests;
        }
    }
    
}

// int main(void) {
//     rcc_setup();
//     error_t err;
    
//     rcc_periph_clock_enable(RCC_GPIOC);
//     gpio_mode_setup(ERROR_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, ERROR_LED_PIN);
//     gpio_clear(ERROR_LED_PORT, ERROR_LED_PIN);
    
//     if ((err = dspi_setup())) {
//         error_handle(err);
//         return err;
//     }
//     if ((err = duart_setup())) {
//         error_handle(err);
//         return err;
//     }
//     if ((err = dmetro_setup())) {
//         error_handle(err);
//         return err;
//     }

//     if ((err = sys_setup(10)))
//         return error_handle(err);
    
//     nvic_set_priority(NVIC_TIM4_IRQ, 2);
//     nvic_set_priority(NVIC_USART2_IRQ, 1);
    

//     uint64_t listening_period;
//     /* flag for detecting packets */
//     bool found;

//     /* flag used to wait for arguments to a command */
//     volatile bool waiting = false;

//     listening_period = 50; // ms

//     while (1) {
//         if ((err = duart_poll_packet(&commandp, 100, &found)))
//             return error_handle(err);
//         if (found) {
//             char *ptr, *cmp;
//             ptr = (char*) command_buf;
//             cmp = commandp.id;
//             volatile int ans;
//             ans = 1;

//             while (*ptr++ == *cmp++)
//                 ans = (*ptr == '\0') ? 0: *ptr - *cmp;  
//             /* strcmp returns 0, so I mimicked that */
//             if (!ans) {
//                 waiting = true;
//             }
//             else if (ans && waiting) {
//                 /* @todo process beat command */
//                 waiting = false;
//             }

//         }
//         // dspi_rcv(&sample);
//         // send_stamped_sample();

//         dmetro_poll_update((uint64_t) 250); 
//         ;
//     }
//     return 0;
// }
