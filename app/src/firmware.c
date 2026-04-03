// libopencm3 defines
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>        // from LowByteProductions
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

#include "diss-uart.h"

// FreeRTOS defines
// #include "FreeRTOS.h"

#define CS_PORT (GPIOA)
#define CS_PIN (GPIO4)

/******************  timer defines *************/
#define MAX_PSC (uint32_t)((1 << 16) - 1)
// stage 1
#define STAGE1_METRONOME_TIMER          (TIM4)      // TIM4 doesn't have an ETR
#define STAGE1_RCC                      (RCC_TIM4)
#define STAGE1_METRONOME_OUT_PORT       (GPIOB)
#define STAGE1_METRONOME_OUT_PIN        (GPIO6)
// stage 2
#define STAGE2_METRONOME_TIMER          (TIM3)   
#define STAGE2_RCC                      (RCC_TIM3)
#define STAGE2_METRONOME_IN_PORT        (GPIOD)
#define STAGE2_METRONOME_IN_PIN         (GPIO2)
#define STAGE2_METRONOME_OUT_PORT       (GPIOB)
#define STAGE2_METRONOME_OUT_PIN        (GPIO4)



static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_clock_enable(RCC_TIM4);

    /********** spi setup ************/
    // SCLK is A5
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO5);
    // MISO is A6
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO6);
    // MOSI is A7
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO7);
    // CS is A4 
    gpio_mode_setup(CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, CS_PIN);

    // these are given by datasheet, thankfully all in same AF column
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
    /******** endSPI  *****************/


    /********** UART setup ************/
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN); 
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_RX_PIN); 

    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    /******** endUART  *****************/

    /************** TIMER  *****************/

    /* STAGE 1*/
    gpio_mode_setup(
        STAGE1_METRONOME_OUT_PORT, 
        GPIO_MODE_AF, 
        GPIO_PUPD_NONE,     // correct?
        STAGE1_METRONOME_OUT_PIN
    );
    // /* STAGE 2 */
    // gpio_mode_setup(
    //     STAGE2_METRONOME_IN_PORT, 
    //     GPIO_MODE_AF, 
    //     GPIO_PUPD_PULLDOWN,     // correct?
    //     STAGE2_METRONOME_IN_PIN
    // );
    // gpio_mode_setup(
    //     STAGE2_METRONOME_OUT_PORT, 
    //     GPIO_MODE_AF, 
    //     GPIO_PUPD_NONE,     // correct?
    //     STAGE2_METRONOME_OUT_PIN
    // );

    gpio_set_af(STAGE1_METRONOME_OUT_PORT, GPIO_AF2, STAGE1_METRONOME_OUT_PIN);
    // gpio_set_af(STAGE2_METRONOME_IN_PORT, GPIO_AF2, STAGE2_METRONOME_IN_PIN);
    /************** endTIMER  *****************/
}

static void spi_setup(void) {
    spi_init_master(
        SPI1, 
        SPI_CR1_BAUDRATE_FPCLK_DIV_64, 
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1,
        SPI_CR1_DFF_16BIT,
        SPI_CR1_MSBFIRST
    );
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1); // tell stm it is always master
    spi_enable(SPI1);
}

static void spi_rcv(void) {
    // write blank statement to ADC to start transfer (Claude)
    gpio_clear(CS_PORT, CS_PIN);
    spi_xfer(SPI1, 0x0001);
    // set gpio pin high
    gpio_set(CS_PORT, CS_PIN);
}

static inline int bpm_to_hz(float bpm, float *hz) {
    *hz = bpm / 60;
}

static void basic_timer_setup(void) {
    // timer_disable_counter(STAGE1_METRONOME_TIMER);
    // from https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/f4/stm32f4-discovery/timer/timer.c
    // this *should* work
    timer_set_mode(
        STAGE1_METRONOME_TIMER, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );

    timer_disable_preload(STAGE1_METRONOME_TIMER);
    timer_continuous_mode(STAGE1_METRONOME_TIMER);
    timer_set_period(STAGE1_METRONOME_TIMER, 65536);
    timer_enable_counter(STAGE1_METRONOME_TIMER);
}

static void timer_setup(void) {
    // init vars
    int err, psc1, psc2;
    float  base_tempo, hz;

    err = bpm_to_hz(120, &hz);
    if (!err) {;}
        // do something? 

    psc1 = (int) MAX_PSC;
    // round it ? or just truncate?
    // if BPM is 120, PSC2 is about 340
    psc2 = (int) ((float)(42.0 / psc1) / hz);

    // disable both timers before configuration.
    timer_disable_counter(STAGE1_METRONOME_TIMER);
    timer_disable_counter(STAGE2_METRONOME_TIMER);

    // 42 / MAX_PSC = 640.869140625
    // set first stage to generate clock at some defined frequency 
    timer_set_mode(STAGE1_METRONOME_TIMER, (uint32_t) psc1, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);     // edge appropriate?
    
    // set second stage to have input from first stage
    timer_set_mode(STAGE2_METRONOME_TIMER, (uint32_t) psc2, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    // stage 1 will divide the clock to something nice to divide, or both can be done at once?
    timer_slave_set_extclockmode2(STAGE2_METRONOME_TIMER, TIM_ECM2_ENABLED);

    timer_enable_counter(STAGE1_METRONOME_TIMER);
    timer_enable_counter(STAGE2_METRONOME_TIMER);
}

/* might not need to do this ?*/
static void systick_setup(void) {
    // systick_set_frequency(84000000, );
    
}


void delay_cycles(uint32_t cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++) {
        __asm__ volatile ("NOP");
    }
}

static void minimal_timer_setup(void) {
    /* minimum example to set TIM4 up (STAGE1) */
    // init clock
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
    
    // gpio
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);    // LED2
    gpio_set(GPIOA, GPIO5);

    // try internal first, from example
    // gpio_set_af(STAGE1_METRONOME_OUT_PORT, GPIO_AF2, STAGE1_METRONOME_OUT_PIN);

    //timer setup
    rcc_periph_clock_enable(STAGE1_RCC);
    nvic_enable_irq(NVIC_TIM4_IRQ);     // stage 1 is TIM4

    timer_set_mode(
        STAGE1_METRONOME_TIMER, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );

    timer_set_prescaler(STAGE1_METRONOME_TIMER, MAX_PSC);

    timer_disable_preload(STAGE1_METRONOME_TIMER);
    timer_continuous_mode(STAGE1_METRONOME_TIMER);

    timer_set_period(STAGE1_METRONOME_TIMER, 65535);

    timer_set_oc_value(STAGE1_METRONOME_TIMER, TIM_OC1, 65535/ 2);

    timer_enable_counter(STAGE1_METRONOME_TIMER);
    timer_enable_irq(STAGE1_METRONOME_TIMER, TIM_DIER_CC1IE);

}

void tim4_isr(void) {
    if (timer_get_flag(STAGE1_METRONOME_TIMER, TIM_SR_CC1IF)) {
        timer_clear_flag(STAGE1_METRONOME_TIMER, TIM_SR_CC1IF);
        gpio_toggle(GPIOA, GPIO5);
    }
}

int main(void) {
    minimal_timer_setup();
    while (1) {
        // delay_cycles(84000000 / 2);     // 0.5 seconds
        ;
    }
}

// int main(void) {
//     rcc_setup();
//     gpio_setup();
//     spi_setup();
//     uart_setup();
//     basic_timer_setup;
//     // timer_setup();

//     uint8_t test_message[30] = "Hello, world!";     // remember weird string literal rules
//     uint8_t data;
//     int err;

//     while (1) {
//         // busy work
//         delay_cycles(84000000 / 4);
//         // uart_read(data);
//         // uart_write(test_message);
        
//         // if ((err = uart_read(&data)) == 0)
//         //     break;
//     }
//     return err;
// }
